#pragma once

#include <string>
#include <vector>
#include <cmath>

#include <QObject>

class Model;
struct Node;
 
class AutoWire : public QObject
{
    Q_OBJECT
public:
	AutoWire(Model* model, double wireGap) : QObject(nullptr),
        m_model( model ), m_wireGap( wireGap )
    { }

    void WireModel( double startX, double startY);

    [[nodiscard]] bool GetWorked() { return m_worked; }
    [[nodiscard]] bool GetCancelled() { return m_cancelled; }
    void Cancel() {
        m_cancelled = true;
    }

    [[nodiscard]] std::vector< int > GetIndexes() { return m_doneIndexs; }

    [[nodiscard]] std::vector< Node* > GetDoneNodes() { return m_doneNodes; }

    void WireModel2(double startX, double startY);

signals:
    void OnProgressSent(int, int);

private:
    Model* m_model;
    double m_wireGap{ 500.0 };
    std::vector< int > m_doneIndexs;
    std::vector< Node* > m_doneNodes;
    bool m_worked { false };
    bool m_cancelled{ false };
    void WireNode( std::vector< Node > nodes, int count, std::vector< int > wiredIndex );
    void WireNode2(Node* node, int count, int total, std::vector< Node* > wiredIndex);
};
