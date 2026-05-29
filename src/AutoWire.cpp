#include "AutoWire.h"

#include "Model.h"
#include "Node.h"
#include "model_utils.h"

#include <algorithm>
#include <utility>

namespace
{
    // Safety cap on recursive steps so an impossible/huge search can't hang the UI.
    constexpr long long kMaxSteps = 5'000'000;
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
    m_worked = false;
    m_steps  = 0;

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
    if( m_worked ) {
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

    if( ++m_steps > kMaxSteps ) {
        return;  // give up the exhaustive search; keep the best path so far
    }

    int const current = path.back();

    // Warnsdorff's heuristic: try the next node with the fewest still-unwired
    // neighbours first. This finds a complete path quickly and avoids stranding
    // nodes; m_neighbors is nearest-first so ties prefer the closer node.
    std::vector< std::pair< int, int > > candidates;  // (onward degree, node)
    for( int next : m_neighbors[ current ] ) {
        if( visited[ next ] ) {
            continue;
        }
        int onward = 0;
        for( int nn : m_neighbors[ next ] ) {
            if( !visited[ nn ] ) {
                ++onward;
            }
        }
        candidates.emplace_back( onward, next );
    }
    std::stable_sort( candidates.begin(), candidates.end(),
                      []( auto const& a, auto const& b ) { return a.first < b.first; } );

    for( auto const& [ onward, next ] : candidates ) {
        visited[ next ] = true;
        path.push_back( next );
        WireNode( visited, path );
        path.pop_back();
        visited[ next ] = false;
        if( m_worked ) {
            return;
        }
    }
}
