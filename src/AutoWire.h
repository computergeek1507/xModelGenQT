#pragma once

#include <vector>

class Model;
struct Node;

// Finds a wiring order through the model's nodes by recursive backtracking.
// Starting from a chosen node it hops to any not-yet-wired node within wireGap
// (so it may skip a near node and pick it up later), trying alternatives until it
// finds a path that visits every node. If no complete path exists within the gap,
// the longest path found is kept.
class AutoWire
{
public:
    AutoWire( Model* model, double wireGap ) :
        m_model( model ), m_wireGap( wireGap )
    { }

    void WireModel( double startX, double startY );

    [[nodiscard]] bool GetWorked() { return m_worked; }

    [[nodiscard]] std::vector< int > GetIndexes() { return m_doneIndexs; }

private:
    Model* m_model;
    double m_wireGap{ 5.0 };
    std::vector< int > m_doneIndexs;  // best (longest) path found; complete if m_worked
    bool   m_worked{ false };

    // Within-gap neighbours of each node, nearest first.
    std::vector< std::vector< int > > m_neighbors;
    long long m_steps{ 0 };

    void WireNode( std::vector< bool >& visited, std::vector< int >& path );
};
