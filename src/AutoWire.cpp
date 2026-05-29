#include "AutoWire.h"

#include "Model.h"
#include "Node.h"
#include "model_utils.h"

#include <algorithm>

void AutoWire::WireModel( double startX, double startY )
{
    std::vector< Node > const& nodes = m_model->GetNodes();

    int index = m_model->FindNodeIndex( startX, startY );

    if( index == -1 ) {
        return;
    }

    WireNode( nodes, index );
}

// Greedy nearest-neighbour wiring: start at the chosen node and repeatedly hop to
// the closest not-yet-wired node that lies within m_wireGap, until no reachable
// node remains. m_wireGap is expressed in the model's drawing units (the caller is
// responsible for converting a real-world gap into those units). This runs in
// O(n^2) instead of the previous exponential backtracking search.
//
// The partial order is always recorded in m_doneIndexs; m_worked is only true when
// every node was reached, so the caller can detect a gap that is too small.
void AutoWire::WireNode( std::vector< Node > const& nodes, int startIndex )
{
    if( nodes.empty() ) {
        return;
    }

    std::vector< bool > visited( nodes.size(), false );

    std::vector< int > order;
    order.reserve( nodes.size() );
    order.push_back( startIndex );
    visited[ startIndex ] = true;

    int current = startIndex;

    while( order.size() < nodes.size() ) {
        int    nearest     = -1;
        double nearestDist = 0.0;

        for( int i = 0; i < static_cast< int >( nodes.size() ); ++i ) {
            if( visited[ i ] ) {
                continue;
            }

            double dist = model_utils::GetDistance( nodes[ current ], nodes[ i ] );
            if( dist > m_wireGap ) {
                continue;  // too far to wire to from the current node
            }
            if( nearest == -1 || dist < nearestDist ) {
                nearest     = i;
                nearestDist = dist;
            }
        }

        if( nearest == -1 ) {
            break;  // no unwired node within the gap — wiring is stuck here
        }

        order.push_back( nearest );
        visited[ nearest ] = true;
        current            = nearest;
    }

    m_doneIndexs = order;
    m_worked     = ( order.size() == nodes.size() );
}