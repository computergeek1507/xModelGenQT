#pragma once

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QString>
#include <QPointF>

class BoxGraphicsScene : public QGraphicsScene
{
	Q_OBJECT
public:
	BoxGraphicsScene(  QObject* parent = 0 );

	virtual ~BoxGraphicsScene();

Q_SIGNALS:
	// void ShowProgress() const;
	void AddDevice(QString path, QPointF scenePos);

protected:

	virtual void mousePressEvent( QGraphicsSceneMouseEvent* mouseEvent );

	//Drag and drop events
	virtual void dragEnterEvent( QGraphicsSceneDragDropEvent* event );
	virtual void dragLeaveEvent( QGraphicsSceneDragDropEvent* event );
	virtual void dragMoveEvent( QGraphicsSceneDragDropEvent* event );
	virtual void dropEvent( QGraphicsSceneDragDropEvent* event ) override;
	
	QGraphicsItem* m_dragItem;
};