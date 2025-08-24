#pragma once

#include "Node.h"

#include <string>
#include <vector>
#include <optional>
#include <cmath>

class Model
{
public:

    void ClearWiring();

    void ClearNodes() { m_nodes.clear(); }

    void ExportModel( std::string const& filename, int grid_width, int grid_heigth );

    void SetBoundingBox( int minX, int maxX, int minY, int maxY );

    void ScaleNodesToGrid( int grid_width, int grid_heigth);

    void AddNode( Node _node );

    void SortNodes();

    [[nodiscard]] std::optional< std::reference_wrapper< Node > > GetNode( int i )
    {
        if (i > m_nodes.size()) {
            return std::nullopt;
        }
        return m_nodes.at( i );
    }


    [[nodiscard]] std::vector< Node > const& GetNodes() const
    {
        return m_nodes;
    }

    [[nodiscard]] std::vector< Node > & GetEditNodes()
    {
        return m_nodes;
    }

    [[nodiscard]] size_t GetNodeCount() const { return m_nodes.size(); }

    [[nodiscard]] std::optional< std::reference_wrapper< Node > > FindNode( int x, int y )
    {
        if( auto found{ std::find_if( m_nodes.begin(), m_nodes.end(), [ &x, &y ]( Node const& n ) { return n.X == x && n.Y == y; } ) };
            found != m_nodes.end() ) {
            return *found;
        }

        return std::nullopt;
    }

    //[[nodiscard]] std::optional< std::reference_wrapper< Node > > FindGridNode( int x, int y )
    //{
    //    if( auto found{ std::find_if( m_nodes.begin(), m_nodes.end(), [ &x, &y ]( Node const& n ) { return n.GridX == x && n.GridY == y; } ) };
    //        found != m_nodes.end() ) {
    //        return *found;
    //    }
    //
    //    return std::nullopt;
    //}

    [[nodiscard]] std::optional< std::reference_wrapper< Node > > FindNodeNumber( int nodeNumber )
    {
        if( auto found{ std::find_if( m_nodes.begin(), m_nodes.end(), [ &nodeNumber ]( Node const& n ) { return n.NodeNumber == nodeNumber; } ) };
            found != m_nodes.end() ) {
            return *found;
        }

        return std::nullopt;
    }

    [[nodiscard]] int FindNodeIndex( int x, int y ) const
    {
        int i = 0;
        for( auto const& node : m_nodes ) {
            if( x == node.X && y == node.Y ) {
                return i;
            }
            i++;
        }
        return -1;
    }

    void DeleteNode( int x, int y )
    {
        int idx = FindNodeIndex( x, y );
        if( idx == -1 ) {
            return;
        }

        m_nodes.erase( m_nodes.begin() + idx );
    }

    void SetNodeNumber( int x, int y, int node )
    {
        int idx = FindNodeIndex( x, y );
        if( idx == -1 ) {
            return;
        }
        m_nodes.at( idx ).NodeNumber = node;
    }

    void SetNodeNumber( int i, int node )
    {
        if( i > m_nodes.size() ) {
            return;
        }
        m_nodes.at( i ).NodeNumber = node;
    }

    void SetName( std::string name ) { m_name = std::move( name ); }
    [[nodiscard]] std::string const& GetName() const { return m_name; }

private:
    std::vector<Node> m_nodes;
    std::string m_name;
    int m_sizeX{0};
    int m_sizeY{0};

};
