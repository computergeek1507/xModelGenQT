#include "AutoWire.h"

//#include "model_utils.h"

#include "Model.h"
#include "Node.h"


#include <algorithm>

void AutoWire::WireModel(double startX, double startY)
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
    //if( m_worked || m_cancelled )
    //    return;
    //if( count + 1 >= nodes.size() ) {
    //    if( wiredIndex.size() == nodes.size() ) {
    //        m_doneIndexs = wiredIndex;
    //        m_worked     = true;
    //    }
    //    return;
    //}
    //emit OnProgressSent( wiredIndex.size(), count);
    //
    //for( int i = 0; i < nodes.size(); ++i ) {
    //    if( std::find( wiredIndex.begin(), wiredIndex.end(), i ) != wiredIndex.end() ) {
    //        continue;
    //    }
    //
    //    double dist = model_utils::GetDistance( nodes[ wiredIndex[ count ] ], nodes[ i ] );
    //    if( dist <= m_wireGap ) {
    //        std::vector< int > newwiredIndex = std::vector< int >( wiredIndex );
    //        newwiredIndex.push_back( i );
    //        int newCount = count;
    //        newCount++;
    //        WireNode( nodes, newCount, newwiredIndex );
    //    }
    //}
}

void AutoWire::WireModel2(double startX, double startY)
{
	m_model->FindCloseNodes(m_wireGap);
    //OnListSizeSent( _model.GetNodeCount() + 1 );

    //std::vector< Node > Nodes = std::vector< Node >(m_model->GetNodes());

    auto node = m_model->FindNode(startX, startY);

    if (!node) {
        return;
    }

    std::vector<  Node* > nodesUsed = std::vector<  Node* >();
    nodesUsed.push_back(&node->get());
    int count = 0;

    
    //for()

    WireNode2(&node->get(), count, m_model->GetNodeCount(), nodesUsed);
}

void AutoWire::WireNode2( Node* node, int count,int total,  std::vector< Node* > wiredIndex)
{
    if (m_cancelled && m_doneNodes.empty())
    {
        m_doneNodes = wiredIndex;
        return;
    }

    if (m_worked || m_cancelled)
        return;
    if (count + 1 >= total) {
        if (wiredIndex.size() == total) {
            m_doneNodes = wiredIndex;
            m_worked = true;
        }
        return;
    }
    emit OnProgressSent(wiredIndex.size(), count);

    for (auto* nn : node->CloseNodes) {

        if (std::find(wiredIndex.begin(), wiredIndex.end(), nn) != wiredIndex.end()) {
            continue;
		}
        std::vector<  Node* > newwiredIndex = wiredIndex;
        newwiredIndex.push_back(nn);
        int newCount = count;
        newCount++;
        WireNode2(nn, newCount, total,  newwiredIndex);
    }
}