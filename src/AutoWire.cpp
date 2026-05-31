#include "AutoWire.h"

#include "Model.h"
#include "Node.h"
#include "model_utils.h"

#include <algorithm>
#include <utility>

namespace
{
    // Warnsdorff reliably completes in a few hundred steps, so a high cap only ever
    // bites on genuinely impossible gaps.
    constexpr long long kWarnsdorffMaxSteps = 5'000'000;

    // Nearest-first solves easy start nodes almost immediately but can chase an
    // unsolvable greedy trap forever. Bound it low so it gives up quickly (keeping
    // the best partial path) instead of freezing - the user can switch to Warnsdorff.
    constexpr long long kNearestMaxSteps = 500'000;
}

void AutoWire::WireModel( double startX, double startY )
{
    std::vector< Node > const& nodes = m_model->GetNodes();
    int const                  start = m_model->FindNodeIndex( startX, startY );
    if( start == -1 ) {
        return;
    }

    int const n = static_cast< int >( nodes.size() );

    // Precompute every node's within-gap neighbours, nearest first. Trying these in
    // order finds a complete path quickly when one exists, while still allowing the
    // search to skip a near node (go every other) and backtrack when it dead-ends.
    m_neighbors.assign( n, {} );
    for( int i = 0; i < n; ++i ) {
        std::vector< std::pair< double, int > > near;
        for( int j = 0; j < n; ++j ) {
            if( j == i ) {
                continue;
            }
            double const d = model_utils::GetDistance( nodes[ i ], nodes[ j ] );
            if( d <= m_wireGap ) {
                near.emplace_back( d, j );
            }
        }
        std::sort( near.begin(), near.end() );
        m_neighbors[ i ].reserve( near.size() );
        for( auto const& [ d, j ] : near ) {
            m_neighbors[ i ].push_back( j );
        }
    }

    m_doneIndexs.clear();
    m_worked   = false;
    m_canceled = false;
    m_steps    = 0;
    m_maxSteps = ( m_strategy == Strategy::Warnsdorff ) ? kWarnsdorffMaxSteps
                                                        : kNearestMaxSteps;

    if( n == 0 ) {
        return;
    }

    std::vector< bool > visited( n, false );
    std::vector< int >  path;
    path.reserve( n );
    visited[ start ] = true;
    path.push_back( start );
    m_doneIndexs = path;  // best path so far

    WireNode( visited, path );
}

void AutoWire::WireNode( std::vector< bool >& visited, std::vector< int >& path )
{
    if( m_worked || m_canceled ) {
        return;
    }

    // Remember the longest path found, in case no complete path exists.
    if( path.size() > m_doneIndexs.size() ) {
        m_doneIndexs = path;
    }

    if( path.size() == m_neighbors.size() ) {
        m_worked = true;  // visited every node
        return;
    }

    if( ++m_steps > m_maxSteps ) {
        return;  // give up the exhaustive search; keep the best path so far
    }

    // Periodically report progress and honour a cancel request. Throttled so the
    // callback (which pumps the UI event loop) doesn't dominate the search cost, but
    // often enough that the Cancel button stays responsive - even in a Debug build,
    // where each step is much slower, the window must pump frequently or Windows
    // flags it "not responding" and stops delivering the click.
    if( m_progress && ( m_steps & 0x7FF ) == 0 ) {
        if( !m_progress( static_cast< int >( m_doneIndexs.size() ),
                         static_cast< int >( m_neighbors.size() ), m_steps ) ) {
            m_canceled = true;
            return;
        }
    }

    int const current = path.back();

    // Build the order in which to try the unwired neighbours. m_neighbors is
    // presorted nearest-first, so the rank in that list is each candidate's
    // distance order; we keep it as a tie-breaker either way.
    std::vector< std::pair< int, int > > cand;  // (sortKey, node)
    for( int next : m_neighbors[ current ] ) {
        if( visited[ next ] ) {
            continue;
        }
        // `current` is still marked visited, so this onward count already excludes
        // the node we'd be arriving from.
        int onward = 0;
        for( int nn : m_neighbors[ next ] ) {
            if( !visited[ nn ] ) {
                ++onward;
            }
        }

        int key;
        if( m_strategy == Strategy::Warnsdorff ) {
            // Fewest onward moves first (distance breaks ties via `rank`).
            key = onward;
        } else {
            // Nearest-first, but promote "forced" moves: a neighbour left with at
            // most one unwired neighbour must be taken soon or it gets stranded.
            // Everything else keeps strict closest-first order.
            key = ( onward <= 1 ) ? 0 : 1;
        }
        cand.emplace_back( key, next );
    }

    // Stable by construction is enough: cand was built in nearest-first order, so a
    // stable sort on the key preserves distance order within equal keys.
    std::stable_sort( cand.begin(), cand.end(),
                      []( auto const& a, auto const& b ) { return a.first < b.first; } );

    for( auto const& [ key, next ] : cand ) {
        visited[ next ] = true;
        path.push_back( next );
        // Prune branches that can never complete: if hopping to `next` strands
        // any still-unwired node in an unreachable pocket, no full path follows.
        // This is what stops the search blowing up at small wire gaps, where the
        // graph is sparse and most branches dead-end.
        if( AllReachable( visited, next ) ) {
            WireNode( visited, path );
        }
        path.pop_back();
        visited[ next ] = false;
        if( m_worked || m_canceled ) {
            return;
        }
    }
}

bool AutoWire::AllReachable( std::vector< bool > const& visited, int from ) const
{
    int const n = static_cast< int >( visited.size() );

    int unvisited = 0;
    for( int i = 0; i < n; ++i ) {
        if( !visited[ i ] ) {
            ++unvisited;
        }
    }
    if( unvisited == 0 ) {
        return true;
    }

    std::vector< bool > seen( n, false );
    std::vector< int >  stack;
    for( int nb : m_neighbors[ from ] ) {
        if( !visited[ nb ] && !seen[ nb ] ) {
            seen[ nb ] = true;
            stack.push_back( nb );
        }
    }

    int reached = 0;
    while( !stack.empty() ) {
        int const node = stack.back();
        stack.pop_back();
        ++reached;
        for( int nb : m_neighbors[ node ] ) {
            if( !visited[ nb ] && !seen[ nb ] ) {
                seen[ nb ] = true;
                stack.push_back( nb );
            }
        }
    }

    return reached == unvisited;
}
