#include "BoxGraphicsView.h"

//Qt includes
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTextStream>
#include <QWheelEvent>

/**
* Sets up the subclassed QGraphicsView
*/
BoxGraphicsView::BoxGraphicsView( QWidget* parent ) :
    QGraphicsView( parent )
{

    setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );

    ////Set-up the scene
    //QGraphicsScene* Scene = new QGraphicsScene(this);
    //setScene(Scene);

    ////Populate the scene
    //for(int x = 0; x < 1000; x = x + 25) {
    //    for(int y = 0; y < 1000; y = y + 25) {

    //        if(x % 100 == 0 && y % 100 == 0) {
    //            Scene->addRect(x, y, 2, 2);

    //            QString pointString;
    //            QTextStream stream(&pointString);
    //            stream << "(" << x << "," << y << ")";
    //            QGraphicsTextItem* item = Scene->addText(pointString);
    //            item->setPos(x, y);
    //        } else {
    //            Scene->addRect(x, y, 1, 1);
    //        }
    //    }
    //}

    ////Set-up the view
    //setSceneRect(0, 0, 1000, 1000);

    ////Use ScrollHand Drag Mode to enable Panning
	//setDragMode( ScrollHandDrag  );
	setDragMode( RubberBandDrag );
}

/**
  * Zoom the view in and out.
  */
void BoxGraphicsView::wheelEvent( QWheelEvent* event )
{

    setTransformationAnchor( QGraphicsView::AnchorUnderMouse );

    // Scale the view / do the zoom
    double scaleFactor{ 1.15 };
    if( event->angleDelta().y() > 0 )
    {
        // Zoom in
        scale( scaleFactor, scaleFactor );
    }
    else
    {
        // Zooming out
        scale( 1.0 / scaleFactor, 1.0 / scaleFactor );
    }

    // Don't call superclass handler here
    // as wheel is normally used for moving scrollbars
}

void BoxGraphicsView::mousePressEvent( QMouseEvent* e )
{
	if( e->button() == Qt::LeftButton )
	{
		origin = e->pos();
		if( !rubberBand )
		{
			rubberBand = new QRubberBand( QRubberBand::Rectangle, this );
		}
		rubberBand->setGeometry( QRect( origin, QSize() ) );
		rubberBand->show();
	}
    //if (e->button() == Qt::RightButton)
    //{
    //	//rubberBandOrigin = event->pos();
    //	Q_EMIT MouseSelectSignal(e->pos());
    //
    //}
    QGraphicsView::mousePressEvent( e );
}

void BoxGraphicsView::mouseMoveEvent( QMouseEvent* event )
{
	if( rubberBand )
	{
		rubberBand->setGeometry( QRect( origin, event->pos() ).normalized() );
	}

	QGraphicsView::mouseMoveEvent( event );
}

void BoxGraphicsView::mouseReleaseEvent( QMouseEvent* e )
{
	if( rubberBand )
	{
		rubberBand->hide();
		QRect selectionRect = mapToScene( rubberBand->geometry() ).boundingRect().toRect();
		
		QRect selectionRect2 = rubberBand->geometry();
		emit MouseSelectRectSignal( selectionRect );
		// Select items within the selection area
		//QList< QGraphicsItem* > selectedItems = scene()->items( mapToScene( selectionRect ) );
		//for( auto item : selectedItems )
		//{
		//	// Perform the desired action on selected items
		//	// For example, set the item's selected state:
		//	item->setSelected( true );
		//}
	}
    QGraphicsView::mouseReleaseEvent( e );
}
