#include "AutoWire.h"

#include "Model.h"
#include "Node.h"
#include "model_utils.h"

#include <algorithm>

void AutoWire::WireModel( int startX, int startY )
{
    //OnListSizeSent( _model.GetNodeCount() + 1 );

    std::vector< Node > Nodes = std::vector< Node >( m_model->GetNodes() );

    int index = m_model->FindNodeIndex( startX, startY );

    if( index == -1 ) {
        return;
    }

    std::vector< int > nodesUsed = std::vector< int >();
    nodesUsed.push_back( index );
    int count = 0;

    WireNode( Nodes, count, nodesUsed );
}

void AutoWire::WireNode( std::vector< Node > nodes, int count, std::vector< int > wiredIndex )
{
    if( m_worked )
        return;
    if( count + 1 >= nodes.size() ) {
        if( wiredIndex.size() == nodes.size() ) {
            m_doneIndexs = wiredIndex;
            m_worked     = true;
        }
        return;
    }
    //OnProgressSent( wiredIndex.Count().ToString(), count );

    for( int i = 0; i < nodes.size(); ++i ) {
        if( std::find( wiredIndex.begin(), wiredIndex.end(), i ) != wiredIndex.end() ) {
            continue;
        }
        //if( wiredIndex.Contains( i ) )
        //    continue;

        double dist = model_utils::GetDistance( nodes[ wiredIndex[ count ] ], nodes[ i ] );
        if( dist <= m_wireGap ) {
            std::vector< int > newwiredIndex = std::vector< int >( wiredIndex );
            newwiredIndex.push_back( i );
            int newCount = count;
            newCount++;
            WireNode( nodes, newCount, newwiredIndex );
        }
    }
}