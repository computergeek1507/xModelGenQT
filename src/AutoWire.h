#pragma once

#include <functional>
#include <vector>

class Model;
struct Node;

// Finds a wiring order through the model's nodes by recursive backtracking.
// Starting from a chosen node it hops to any not-yet-wired node within wireGap
// (so it may skip a near node and pick it up later), trying alternatives until it
// finds a path that visits every node. If no complete path exists within the gap,
// the longest path found is kept.
//
// Two move-ordering strategies trade tidiness against reliability:
//   NearestFirst - always try the closest unwired node first. Gives the tidiest
//       (shortest-hop) wiring, but can fail to complete from some start nodes
//       because it greedily strands the perimeter; bounded by a step budget so it
//       gives up quickly instead of grinding.
//   Warnsdorff   - try the node with the fewest onward moves first (distance only
//       breaks ties). Reliably completes from almost any start, at the cost of
//       longer hops.
class AutoWire
{
public:
    enum class Strategy { NearestFirst, Warnsdorff };

    AutoWire( Model* model, double wireGap ) :
        m_model( model ), m_wireGap( wireGap )
    { }

    void SetStrategy( Strategy s ) { m_strategy = s; }

    // Optional progress/cancel hook. Called periodically during the (potentially
    // long) search with the best path length found so far, the total node count and
    // the step counter. Return false to abort early; the best path so far is kept.
    // Lets the UI show a progress dialog and stay responsive without a worker thread.
    using ProgressFn = std::function< bool( int bestLen, int total, long long steps ) >;
    void SetProgressCallback( ProgressFn cb ) { m_progress = std::move( cb ); }

    void WireModel( double startX, double startY );

    [[nodiscard]] bool GetWorked() { return m_worked; }

    [[nodiscard]] std::vector< int > GetIndexes() { return m_doneIndexs; }

private:
    Model* m_model;
    double m_wireGap{ 5.0 };
    Strategy m_strategy{ Strategy::NearestFirst };
    std::vector< int > m_doneIndexs;  // best (longest) path found; complete if m_worked
    bool   m_worked{ false };

    // Within-gap neighbours of each node, nearest first.
    std::vector< std::vector< int > > m_neighbors;
    long long m_steps{ 0 };
    long long m_maxSteps{ 0 };  // step budget for the current run (set in WireModel)

    ProgressFn m_progress;          // optional; see SetProgressCallback
    bool       m_canceled{ false }; // set when m_progress asks to abort

    void WireNode( std::vector< bool >& visited, std::vector< int >& path );

    // True if every still-unwired node is reachable from `from` travelling only
    // through other unwired nodes (a flood fill over the remaining subgraph).
    [[nodiscard]] bool AllReachable( std::vector< bool > const& visited, int from ) const;
};
