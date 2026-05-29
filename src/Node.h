#pragma once

#include <string>
#include <vector>

struct Node 
{
    Node() = default;

    Node(double x, double y, double r) :
		X(x), Y(y), Radius(r)
    {}

    inline bool operator==( const Node& lhs) { return lhs.X == X && lhs.Y == Y; }

    double X{ -1.0 };
    double Y{ -1.0 };
    double Radius{ 1.0 };
    int NodeNumber{0};
    //bool fir

    std::vector< Node* > CloseNodes;

    void AddCloseNode(Node* n ) 
    {
        CloseNodes.push_back(n);
    }

    [[nodiscard]] bool IsWired() const { return NodeNumber != 0; }
    [[nodiscard]] bool IsFirst() const { return NodeNumber == 1; }

    [[nodiscard]] std::string GetText() const
    {
        return "Node:" + std::to_string( NodeNumber ) + "     X:" + std::to_string( X ) + "     Y:" + std::to_string( Y );
    }

    void ClearWiring()
    {
        NodeNumber = 0;
    }

    void ClearCloseNodes()
    {
        CloseNodes.clear();
    }
};
