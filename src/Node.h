#pragma once

#include <string>

struct Node 
{
    Node() = default;

    Node(double x, double y ) :
        X( x ), Y( y )
    {}

    inline bool operator==( const Node& lhs) { return lhs.X == X && lhs.Y == Y; }

    double X{ -1.0 };
    double Y{ -1.0 };
    //int GridX{ -1 };
    //int GridY{ -1 };
    int NodeNumber{0};

    [[nodiscard]] bool IsWired() const { return NodeNumber != 0; }

    [[nodiscard]] std::string GetText() const
    {
        return "Node:" + std::to_string( NodeNumber ) + "     X:" + std::to_string( X ) + "     Y:" + std::to_string( Y );
    }

    void ClearWiring()
    {
        NodeNumber = 0;
    }
};
