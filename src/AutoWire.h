#pragma once

#include <string>
#include <vector>
#include <cmath>

class Model;
struct Node;

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
    std::vector< int > m_doneIndexs;
    bool m_worked { false };

    void WireNode( std::vector< Node > const& nodes, int startIndex );
};
