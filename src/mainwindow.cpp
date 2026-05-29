#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "AutoWire.h"
#include "dxf/dxf_reader.h"
#include "dxf/dxf_units.h"
#include "dxf/hole_finder.h"

#include <QBrush>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QInputDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPen>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <limits>
#include <memory>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_DXF_triggered()
{
    QString const fileName = QFileDialog::getOpenFileName(
        this, tr( "Open DXF" ), QString(), tr( "DXF files (*.dxf);;All files (*)" ) );
    if( fileName.isEmpty() ) {
        return;  // user cancelled
    }

    loadDxf( fileName );
}

void MainWindow::loadDxf( QString const& fileName )
{
    spdlog::info( "Opening DXF: {}", fileName.toStdString() );

    dxf_reader reader;
    if( !reader.openFile( fileName.toStdString() ) ) {
        spdlog::error( "Failed to read DXF: {}", fileName.toStdString() );
        QMessageBox::warning( this, tr( "Open DXF" ),
                              tr( "Failed to read \"%1\"." ).arg( fileName ) );
        return;
    }

    m_dxf_data = reader.moveData();
    m_modelName = QFileInfo( fileName ).baseName().toStdString();
    spdlog::debug( "DXF model-space: {} circles, {} arcs, {} polylines, {} lines, {} inserts; "
                   "{} blocks (units code {})",
                   m_dxf_data->model.circles.size(), m_dxf_data->model.arcs.size(),
                   m_dxf_data->model.polylines.size(), m_dxf_data->model.lines.size(),
                   m_dxf_data->model.inserts.size(), m_dxf_data->blocks.size(),
                   m_dxf_data->insUnits );

    detectHoles();
}

void MainWindow::setHoleDiameter( double mm )
{
    ui->doubleSpinBox_holeDia->setValue( mm );
}

double MainWindow::mmToDrawingUnits( double mm ) const
{
    if( !m_dxf_data ) {
        return mm;
    }
    double const du =
        dxf_units::ToDrawingUnits( mm, dxf_units::Millimeters, m_dxf_data->insUnits );
    return du < 0.0 ? mm : du;  // unknown units: treat the drawing as millimeters
}

void MainWindow::detectHoles()
{
    if( !m_dxf_data ) {
        return;
    }

    // The target hole diameter is a real-world size (mm) from the spin box, plus a
    // fixed +/-0.5mm band on the radius (i.e. +/-1mm on the diameter). Detection runs
    // in the file's drawing units; block references are expanded to world space.
    double const holeDiameterMm  = ui->doubleSpinBox_holeDia->value();
    double const holeDiameter    = mmToDrawingUnits( holeDiameterMm );
    double const radiusTolerance = mmToDrawingUnits( 0.5 );

    std::vector< hole_finder::Hole > const holes =
        hole_finder::FindHoles( *m_dxf_data, holeDiameter, radiusTolerance );

    // Node coordinates are stored in millimetres (Node uses double coordinates).
    double mmPerUnit = dxf_units::MillimetersPerUnit( m_dxf_data->insUnits );
    if( mmPerUnit <= 0.0 ) {
        mmPerUnit = 1.0;  // unknown units: treat the drawing as millimetres
    }

    auto model = std::make_unique< Model >();
    model->SetName( m_modelName );
    for( auto const& hole : holes ) {
        model->AddNode(
            Node( hole.x * mmPerUnit, hole.y * mmPerUnit, holeDiameterMm * 0.5 ) );
    }

    spdlog::info( "Found {} hole candidates ({} unique nodes) for {}mm holes",
                  holes.size(), model->GetNodeCount(), holeDiameterMm );

    m_model          = std::move( model );
    m_startNodeIndex = -1;          // selection no longer valid for the new node set
    m_nodeRadius     = holeDiameterMm * 0.5;  // marker radius in mm (node coords are mm)

    refreshModelView();

    if( m_model->GetNodeCount() == 0 ) {
        statusBar()->showMessage(
            tr( "No ~%1mm holes found. Adjust the hole diameter and try again." )
                .arg( holeDiameterMm ) );
    } else {
        statusBar()->showMessage( tr( "Found %1 holes (~%2mm)." )
                                      .arg( static_cast< int >( m_model->GetNodeCount() ) )
                                      .arg( holeDiameterMm ) );
    }
}

void MainWindow::on_doubleSpinBox_holeDia_editingFinished()
{
    // Re-detect holes at the new target diameter on the already-loaded DXF.
    if( m_dxf_data ) {
        detectHoles();
    }
}

void MainWindow::refreshModelView()
{
    ui->listWidgetNodes->clear();
    m_nodeItems.clear();

    if( !m_scene ) {
        m_scene = std::make_unique< QGraphicsScene >();
        ui->graphicsViewDraw->setScene( m_scene.get() );
        // Watch the viewport so clicks/drags can pick the start node.
        ui->graphicsViewDraw->viewport()->installEventFilter( this );
    }
    m_scene->clear();  // deletes the previous markers/labels

    if( !m_model ) {
        return;
    }

    std::vector< Node > const& nodes = m_model->GetNodes();

    // Node list (shows wiring order once the model has been auto-wired).
    for( Node const& node : nodes ) {
        ui->listWidgetNodes->addItem( QString::fromStdString( node.GetText() ) );
    }
    spdlog::info( "Node list populated with {} items", ui->listWidgetNodes->count() );

    // Draw one marker per node. DXF Y points up while scene Y points down, so
    // negate Y to keep the drawing the right way up.
    double const radius = nodeDisplayRadius();
    QPen const   pen( Qt::darkGray );

    m_nodeItems.reserve( nodes.size() );
    for( Node const& node : nodes ) {
        double const cx = node.X;
        double const cy = -node.Y;

        QGraphicsEllipseItem* item = m_scene->addEllipse(
            cx - radius, cy - radius, radius * 2.0, radius * 2.0, pen, QBrush( Qt::yellow ) );
        m_nodeItems.push_back( item );

        // Once wired, label the node with its wiring number.
        if( node.IsWired() ) {
            QGraphicsSimpleTextItem* label =
                m_scene->addSimpleText( QString::number( node.NodeNumber ) );
            // Keep labels a constant on-screen size regardless of zoom.
            label->setFlag( QGraphicsItem::ItemIgnoresTransformations, true );
            label->setBrush( QBrush( Qt::black ) );
            label->setZValue( 1.0 );
            label->setPos( cx + radius, cy - radius );
        }
    }

    updateNodeColors();

    if( !m_scene->items().isEmpty() ) {
        QRectF const bounds = m_scene->itemsBoundingRect();
        m_scene->setSceneRect( bounds );
        ui->graphicsViewDraw->fitInView( bounds, Qt::KeepAspectRatio );
    }
}

void MainWindow::updateNodeColors()
{
    if( !m_model ) {
        return;
    }

    std::vector< Node > const& nodes = m_model->GetNodes();
    for( std::size_t i = 0; i < m_nodeItems.size() && i < nodes.size(); ++i ) {
        if( !m_nodeItems[ i ] ) {
            continue;
        }

        QColor color = Qt::yellow;            // unwired
        if( nodes[ i ].IsWired() ) {
            color = QColor( 90, 170, 255 );   // wired
        }
        if( static_cast< int >( i ) == m_startNodeIndex ) {
            color = QColor( 40, 200, 80 );    // selected start node
        }
        m_nodeItems[ i ]->setBrush( QBrush( color ) );
    }
}

int MainWindow::nearestNodeIndex( double sceneX, double sceneY ) const
{
    if( !m_model ) {
        return -1;
    }

    std::vector< Node > const& nodes = m_model->GetNodes();
    int    nearest     = -1;
    double nearestDist = 0.0;
    for( std::size_t i = 0; i < nodes.size(); ++i ) {
        double const dx   = nodes[ i ].X - sceneX;
        double const dy   = -nodes[ i ].Y - sceneY;  // markers are drawn at -Y
        double const dist = dx * dx + dy * dy;
        if( nearest == -1 || dist < nearestDist ) {
            nearest     = static_cast< int >( i );
            nearestDist = dist;
        }
    }
    return nearest;
}

double MainWindow::nodeDisplayRadius() const
{
    // Draw markers at the detected hole radius so they match the real holes.
    return m_nodeRadius > 0.0 ? m_nodeRadius : 1.0;
}

bool MainWindow::eventFilter( QObject* watched, QEvent* event )
{
    if( watched == ui->graphicsViewDraw->viewport() && m_model && !m_nodeItems.empty()
        && ( event->type() == QEvent::MouseButtonPress
             || event->type() == QEvent::MouseMove ) ) {
        auto* mouse = static_cast< QMouseEvent* >( event );
        if( mouse->buttons() & Qt::LeftButton ) {
            QPointF const scenePos = ui->graphicsViewDraw->mapToScene( mouse->pos() );
            int const     nearest  = nearestNodeIndex( scenePos.x(), scenePos.y() );
            if( nearest >= 0 && nearest != m_startNodeIndex ) {
                m_startNodeIndex = nearest;
                updateNodeColors();
                Node const& n = m_model->GetNodes().at( nearest );
                statusBar()->showMessage(
                    tr( "Start node: (%1, %2)" ).arg( n.X ).arg( n.Y ) );
            }
        }
    }
    return QMainWindow::eventFilter( watched, event );
}

void MainWindow::on_actionExport_xModel_triggered()
{
    if( !m_model || m_model->GetNodeCount() == 0 ) {
        QMessageBox::information( this, tr( "Export xModel" ),
                                 tr( "There is nothing to export. Open a DXF first." ) );
        return;
    }

    bool wired = false;
    for( Node const& n : m_model->GetNodes() ) {
        if( n.IsWired() ) {
            wired = true;
            break;
        }
    }
    if( !wired
        && QMessageBox::question(
               this, tr( "Export xModel" ),
               tr( "The model has not been auto-wired, so the pixel order will just "
                   "follow detection order. Export anyway?" ) )
               != QMessageBox::Yes ) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this, tr( "Export xModel" ),
        QString::fromStdString( m_model->GetName() ) + ".xmodel",
        tr( "xLights model (*.xmodel);;All files (*)" ) );
    if( fileName.isEmpty() ) {
        return;  // user cancelled
    }

    if( m_model->ExportModel( fileName.toStdString() ) ) {
        spdlog::info( "Exported xmodel: {}", fileName.toStdString() );
        statusBar()->showMessage( tr( "Exported \"%1\"." ).arg( QFileInfo( fileName ).fileName() ) );
    } else {
        spdlog::error( "Failed to export xmodel: {}", fileName.toStdString() );
        QMessageBox::warning( this, tr( "Export xModel" ),
                              tr( "Failed to write \"%1\"." ).arg( fileName ) );
    }
}

void MainWindow::on_actionExit_triggered()
{
	
}

void MainWindow::on_actionAutoWire_triggered()
{
    if( !m_model || m_model->GetNodeCount() == 0 ) {
        QMessageBox::information( this, tr( "AutoWire" ),
                                 tr( "There is no model loaded to wire. Open a DXF first." ) );
        return;
    }

    // Node coordinates are in millimetres, so ask for the gap in a real-world unit
    // and convert it to millimetres.
    struct UnitChoice { const char* label; int code; };
    static UnitChoice const unitChoices[] = {
        { "Millimeters", dxf_units::Millimeters },
        { "Centimeters", dxf_units::Centimeters },
        { "Inches",      dxf_units::Inches },
        { "Feet",        dxf_units::Feet },
    };

    QStringList unitItems;
    for( auto const& uc : unitChoices ) {
        unitItems << tr( uc.label );
    }

    bool          ok       = false;
    QString const unitName = QInputDialog::getItem( this, tr( "AutoWire" ),
                                                    tr( "Wire gap units:" ), unitItems,
                                                    0, false, &ok );
    if( !ok ) {
        return;  // user cancelled
    }
    int const realWorldUnit = unitChoices[ unitItems.indexOf( unitName ) ].code;

    double const realWorldGap = QInputDialog::getDouble(
        this, tr( "AutoWire" ),
        tr( "Wire gap (%1):" ).arg( unitName ),
        100.0, 0.0, std::numeric_limits< double >::max(), 3, &ok );
    if( !ok ) {
        return;  // user cancelled
    }

    runAutoWire( realWorldGap * dxf_units::MillimetersPerUnit( realWorldUnit ) );
}

void MainWindow::on_pushButton_autoWire_clicked()
{
    // The spin box gives the wire gap directly in millimetres.
    runAutoWire( ui->spinBox_wireSize->value() );
}

int MainWindow::wireFrom( int startIndex, double wireGapMm )
{
    Node const& start = m_model->GetNodes().at( startIndex );

    spdlog::info( "AutoWire: gap {}mm, start ({}, {})", wireGapMm, start.X, start.Y );

    AutoWire autoWire( m_model.get(), wireGapMm );
    autoWire.WireModel( start.X, start.Y );

    // Write the discovered order back as 1-based node numbers.
    m_model->ClearWiring();
    int nodeNumber = 1;
    for( int index : autoWire.GetIndexes() ) {
        m_model->SetNodeNumber( index, nodeNumber++ );
    }

    m_startNodeIndex = startIndex;
    refreshModelView();

    int const wiredCount = static_cast< int >( autoWire.GetIndexes().size() );
    spdlog::info( "AutoWire wired {}/{} nodes (complete: {})",
                  wiredCount, m_model->GetNodeCount(), autoWire.GetWorked() );
    return wiredCount;
}

void MainWindow::autoWireFromFirst( double wireGapMm )
{
    if( m_model && m_model->GetNodeCount() > 0 ) {
        wireFrom( 0, wireGapMm );
    }
}

void MainWindow::exportModelTo( QString const& fileName )
{
    if( !m_model || m_model->GetNodeCount() == 0 ) {
        return;
    }
    bool const ok = m_model->ExportModel( fileName.toStdString() );
    spdlog::info( "CLI export ({}) -> {}", ok, fileName.toStdString() );
}

void MainWindow::runAutoWire( double wireGapMm )
{
    if( !m_model || m_model->GetNodeCount() == 0 ) {
        QMessageBox::information( this, tr( "AutoWire" ),
                                 tr( "There is no model loaded to wire. Open a DXF first." ) );
        return;
    }

    // The start node is chosen by clicking/dragging in the drawing view.
    if( m_startNodeIndex < 0 || m_startNodeIndex >= static_cast< int >( m_model->GetNodeCount() ) ) {
        QMessageBox::information(
            this, tr( "AutoWire" ),
            tr( "Click a node in the drawing to choose the start node first." ) );
        return;
    }

    int const wiredCount = wireFrom( m_startNodeIndex, wireGapMm );
    int const totalCount = static_cast< int >( m_model->GetNodeCount() );

    if( wiredCount == totalCount ) {
        QMessageBox::information( this, tr( "AutoWire" ),
                                 tr( "Wired all %1 nodes." ).arg( totalCount ) );
    } else {
        QMessageBox::warning(
            this, tr( "AutoWire" ),
            tr( "Only wired %1 of %2 nodes before the gap was too small to continue.\n"
                "Try increasing the wire gap or choosing a different start node." )
                .arg( wiredCount )
                .arg( totalCount ) );
    }
}

void MainWindow::on_actionView_Logs_triggered()
{
	
}
