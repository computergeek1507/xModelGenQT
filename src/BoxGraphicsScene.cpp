#include "BoxGraphicsScene.h"


#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>
#include <QStandardItem>
//#include <QTreeWidgetItem>
#include <QListWidgetItem>

BoxGraphicsScene::BoxGraphicsScene(  QObject* parent ):
m_dragItem( nullptr ),
QGraphicsScene( parent )
{

}

BoxGraphicsScene::~BoxGraphicsScene()
{

}

void BoxGraphicsScene::mousePressEvent( QGraphicsSceneMouseEvent* mouseEvent )
{
	QGraphicsScene::mousePressEvent( mouseEvent );
}

void BoxGraphicsScene::dragEnterEvent( QGraphicsSceneDragDropEvent* event )
{
	//event->ignore();

	if( m_dragItem )
	{
		m_dragItem->setPos( event->scenePos() );
		this->addItem( m_dragItem );
		update();
	}
}

void BoxGraphicsScene::dragLeaveEvent( QGraphicsSceneDragDropEvent* event )
{
	
}

void BoxGraphicsScene::dragMoveEvent( QGraphicsSceneDragDropEvent* event )
{
	
}

void BoxGraphicsScene::dropEvent( QGraphicsSceneDragDropEvent* event )
{
	QListWidgetItem* listWidgetItem = nullptr;

	if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))
	{
		QListWidget* list = dynamic_cast<QListWidget*>(event->source());

		if (nullptr != list)
		{
			QByteArray itemData = event->mimeData()->data("application/x-qabstractitemmodeldatalist");

			QDataStream stream(&itemData, QIODevice::ReadOnly);

			int r, c;
			QMap< int, QVariant > v;
			stream >> r >> c >> v;

			listWidgetItem = list->item(r);
		}
	}

	if (listWidgetItem)
	{
		auto name = listWidgetItem->text();
		auto path = listWidgetItem->data(Qt::UserRole).toString();

		emit AddDevice(path, event->scenePos());
	}
}