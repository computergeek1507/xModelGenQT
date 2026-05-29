#pragma once

#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QRubberBand>
#include <QMouseEvent>

class BoxGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    BoxGraphicsView( QWidget* parent = nullptr );

Q_SIGNALS:
    void MouseSelectSignal( const QPoint& );

    void MouseSelectRectSignal( const QRect& );

protected:
    //Take over the interaction
    virtual void wheelEvent( QWheelEvent* event ) override;

    void mouseMoveEvent( QMouseEvent* event ) override;
    void mousePressEvent( QMouseEvent* e ) override;
    void mouseReleaseEvent( QMouseEvent* e ) override;

public Q_SLOTS:

    private:
	QRubberBand* rubberBand = nullptr;
	QPoint origin;

};

