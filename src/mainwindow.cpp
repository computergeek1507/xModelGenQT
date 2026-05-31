#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "AutoWire.h"
#include "dxf/dxf_reader.h"
#include "dxf/dxf_units.h"
#include "dxf/hole_finder.h"
#include "svg/svg_reader.h"

#include <QBrush>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QInputDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPen>
#include <QProgressDialog>
#include <QSettings>

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
    QSettings     settings;
    QString const lastDir = settings.value( "paths/openDir" ).toString();

    QString const fileName = QFileDialog::getOpenFileName(
        this, tr( "Open DXF" ), lastDir, tr( "DXF files (*.dxf);;All files (*)" ) );
    if( fileName.isEmpty() ) {
        return;  // user cancelled
    }

    settings.setValue( "paths/openDir", QFileInfo( fileName ).absolutePath() );
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
    m_svgCircles.clear();  // DXF is now the active source
    m_modelName = QFileInfo( fileName ).baseName().toStdString();
    spdlog::debug( "DXF model-space: {} circles, {} arcs, {} polylines, {} lines, {} inserts; "
                   "{} blocks (units code {})",
                   m_dxf_data->model.circles.size(), m_dxf_data->model.arcs.size(),
                   m_dxf_data->model.polylines.size(), m_dxf_data->model.lines.size(),
                   m_dxf_data->model.inserts.size(), m_dxf_data->blocks.size(),
                   m_dxf_data->insUnits );

    detectHoles();
}

void MainWindow::on_actionOpen_SVG_triggered()
{
    QSettings     settings;
    QString const lastDir = settings.value( "paths/openDir" ).toString();

    QString const fileName = QFileDialog::getOpenFileName(
        this, tr( "Open SVG" ), lastDir, tr( "SVG files (*.svg);;All files (*)" ) );
    if( fileName.isEmpty() ) {
        return;  // user cancelled
    }

    settings.setValue( "paths/openDir", QFileInfo( fileName ).absolutePath() );
    loadSvg( fileName );
}

void MainWindow::loadSvg( QString const& fileName )
{
    spdlog::info( "Opening SVG: {}", fileName.toStdString() );

    std::vector< svg_reader::Circle > circles = svg_reader::ReadCircles( fileName );
    if( circles.empty() ) {
        QMessageBox::warning(
            this, tr( "Open SVG" ),
            tr( "No <circle>/<ellipse> elements were found in \"%1\"." ).arg( fileName ) );
        return;
    }

    m_svgCircles = std::move( circles );
    m_dxf_data.reset();  // SVG is now the active source
    m_modelName = QFileInfo( fileName ).baseName().toStdString();
    spdlog::debug( "SVG: {} circle candidates", m_svgCircles.size() );

    detectHoles();
}

void MainWindow::setHoleDiameter( double mm )
{
    ui->doubleSpinBox_holeDia->setValue( mm );
}

int MainWindow::effectiveInsUnits() const
{
    // Combo: 0 Auto, 1 mm, 2 cm, 3 in, 4 ft, 5 m. An override wins over $INSUNITS,
    // which is essential for unitless files (e.g. an inch drawing with $INSUNITS=0).
    switch( ui->comboBox_units->currentIndex() ) {
        case 1:  return dxf_units::Millimeters;
        case 2:  return dxf_units::Centimeters;
        case 3:  return dxf_units::Inches;
        case 4:  return dxf_units::Feet;
        case 5:  return dxf_units::Meters;
        default: return m_dxf_data ? m_dxf_data->insUnits : dxf_units::Unitless;
    }
}

double MainWindow::mmToDrawingUnits( double mm ) const
{
    if( !m_dxf_data ) {
        return mm;
    }
    double const du =
        dxf_units::ToDrawingUnits( mm, dxf_units::Millimeters, effectiveInsUnits() );
    return du < 0.0 ? mm : du;  // unknown units: treat the drawing as millimeters
}

void MainWindow::on_comboBox_units_currentIndexChanged( int /*index*/ )
{
    // Re-interpret the loaded drawing at the new units.
    if( m_dxf_data ) {
        detectHoles();
    }
}

void MainWindow::detectHoles()
{
    // The target hole diameter is a real-world size (mm) from the spin box, plus a
    // fixed +/-0.5mm band on the radius. Node coordinates are stored in millimetres.
    double const holeDiameterMm = ui->doubleSpinBox_holeDia->value();

    std::vector< std::pair< double, double > > centresMm;  // hole centres, node-mm
    int rawCount = 0;

    if( m_dxf_data ) {
        // Detection runs in the file's drawing units; blocks are expanded to world.
        double const holeDiameter    = mmToDrawingUnits( holeDiameterMm );
        double const radiusTolerance = mmToDrawingUnits( 0.5 );
        std::vector< hole_finder::Hole > const holes =
            hole_finder::FindHoles( *m_dxf_data, holeDiameter, radiusTolerance );

        double mmPerUnit = dxf_units::MillimetersPerUnit( effectiveInsUnits() );
        if( mmPerUnit <= 0.0 ) {
            mmPerUnit = 1.0;  // unknown units: treat the drawing as millimetres
        }
        rawCount = static_cast< int >( holes.size() );
        for( auto const& hole : holes ) {
            centresMm.emplace_back( hole.x * mmPerUnit, hole.y * mmPerUnit );
        }
    } else if( !m_svgCircles.empty() ) {
        // SVG circles are already in node-mm; match by radius like the DXF flow.
        double const targetRadiusMm = holeDiameterMm * 0.5;
        rawCount = static_cast< int >( m_svgCircles.size() );
        for( svg_reader::Circle const& c : m_svgCircles ) {
            if( std::abs( c.dia * 0.5 - targetRadiusMm ) <= 0.5 ) {
                centresMm.emplace_back( c.x, c.y );
            }
        }
    } else {
        return;  // nothing loaded
    }

    auto model = std::make_unique< Model >();
    model->SetName( m_modelName );
    for( auto const& [ x, y ] : centresMm ) {
        model->AddNode( Node( x, y, holeDiameterMm * 0.5 ) );
    }

    spdlog::info( "Found {} hole candidates ({} unique nodes) for {}mm holes",
                  rawCount, model->GetNodeCount(), holeDiameterMm );

    m_model          = std::move( model );
    m_startNodeIndex = -1;          // selection no longer valid for the new node set
    m_nodeRadius     = holeDiameterMm * 0.5;  // marker radius in mm (node coords are mm)
    m_selection.clear();            // node indices no longer valid for the new set
    m_strokeAdded.clear();
    if( ui->pushButton_wireSection ) {
        ui->pushButton_wireSection->setEnabled( false );
    }

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

void MainWindow::refreshModelView( bool fitView )
{
    ui->listWidgetNodes->clear();
    m_nodeItems.clear();
    m_selRect = nullptr;  // any live rubber-band is about to be cleared with the scene

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

    updateNodeColorsAndSelection();

    if( !m_scene->items().isEmpty() ) {
        QRectF const bounds = m_scene->itemsBoundingRect();
        m_scene->setSceneRect( bounds );
        if( fitView ) {
            ui->graphicsViewDraw->fitInView( bounds, Qt::KeepAspectRatio );
        }
    }
}

void MainWindow::updateNodeColorsAndSelection()
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
        // The start marker only matters while picking it for whole-model Auto Wire.
        if( m_mode == InteractMode::PickStart
            && static_cast< int >( i ) == m_startNodeIndex ) {
            color = QColor( 40, 200, 80 );    // selected start node
        }
        if( m_selection.count( static_cast< int >( i ) ) ) {
            color = QColor( 255, 150, 40 );   // section selection
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
    if( watched != ui->graphicsViewDraw->viewport() || !m_model || m_nodeItems.empty() ) {
        return QMainWindow::eventFilter( watched, event );
    }

    QEvent::Type const type = event->type();
    if( type != QEvent::MouseButtonPress && type != QEvent::MouseMove
        && type != QEvent::MouseButtonRelease ) {
        return QMainWindow::eventFilter( watched, event );
    }

    auto* mouse = static_cast< QMouseEvent* >( event );
    auto  toScene = [ & ]( QPoint const& p ) { return ui->graphicsViewDraw->mapToScene( p ); };

    // ---- Pick start: click/drag sets the whole-model Auto Wire start node. -------
    if( m_mode == InteractMode::PickStart ) {
        if( ( type == QEvent::MouseButtonPress || type == QEvent::MouseMove )
            && ( mouse->buttons() & Qt::LeftButton ) ) {
            QPointF const sp = toScene( mouse->pos() );
            int const nearest = nearestNodeIndex( sp.x(), sp.y() );
            if( nearest >= 0 && nearest != m_startNodeIndex ) {
                m_startNodeIndex = nearest;
                updateNodeColorsAndSelection();
                Node const& n = m_model->GetNodes().at( nearest );
                statusBar()->showMessage( tr( "Start node: (%1, %2)" ).arg( n.X ).arg( n.Y ) );
            }
        }
        return QMainWindow::eventFilter( watched, event );
    }

    // ---- Manual wire: click a node, or drag across nodes, to wire them by hand. --
    if( m_mode == InteractMode::Manual ) {
        if( type == QEvent::MouseButtonPress && ( mouse->button() == Qt::RightButton ) ) {
            undoLastWire();
            return true;
        }
        if( type == QEvent::MouseButtonPress && ( mouse->button() == Qt::LeftButton ) ) {
            m_dragging = true;
            m_strokeAdded.clear();
            QPointF const sp = toScene( mouse->pos() );
            manualAddNode( nearestNodeIndex( sp.x(), sp.y() ) );
            return true;
        }
        if( type == QEvent::MouseMove && m_dragging && ( mouse->buttons() & Qt::LeftButton ) ) {
            QPointF const sp = toScene( mouse->pos() );
            int const idx = nearestNodeIndex( sp.x(), sp.y() );
            if( idx >= 0 && !m_strokeAdded.count( idx ) ) {
                manualAddNode( idx );
            }
            return true;
        }
        if( type == QEvent::MouseButtonRelease ) {
            m_dragging = false;
        }
        return QMainWindow::eventFilter( watched, event );
    }

    // ---- Select section: rubber-band a box, or click nodes, then Wire Section. ---
    if( m_mode == InteractMode::Section ) {
        if( type == QEvent::MouseButtonPress && ( mouse->button() == Qt::LeftButton ) ) {
            m_dragging = true;
            m_pressViewPos = mouse->pos();
            if( !m_selRect ) {
                QPen pen( QColor( 255, 150, 40 ) );
                pen.setStyle( Qt::DashLine );
                pen.setCosmetic( true );  // constant on-screen width regardless of zoom
                m_selRect = m_scene->addRect( QRectF(), pen, QBrush( QColor( 255, 150, 40, 40 ) ) );
                m_selRect->setZValue( 2.0 );
            }
            m_selRect->setRect( QRectF( toScene( m_pressViewPos ), toScene( m_pressViewPos ) ) );
            return true;
        }
        if( type == QEvent::MouseMove && m_dragging && m_selRect ) {
            m_selRect->setRect(
                QRectF( toScene( m_pressViewPos ), toScene( mouse->pos() ) ).normalized() );
            return true;
        }
        if( type == QEvent::MouseButtonRelease && m_dragging ) {
            m_dragging = false;
            bool const additive = ( mouse->modifiers() & Qt::ControlModifier ) != 0;
            bool const isClick =
                ( mouse->pos() - m_pressViewPos ).manhattanLength() < 4;  // no real drag
            if( m_selRect ) {
                m_scene->removeItem( m_selRect );
                delete m_selRect;
                m_selRect = nullptr;
            }
            if( isClick ) {
                // Toggle the single nearest node in/out of the selection.
                QPointF const sp = toScene( mouse->pos() );
                int const idx = nearestNodeIndex( sp.x(), sp.y() );
                if( idx >= 0 ) {
                    if( m_selection.count( idx ) ) {
                        m_selection.erase( idx );
                    } else {
                        m_selection.insert( idx );
                    }
                }
            } else {
                applyRectSelection(
                    QRectF( toScene( m_pressViewPos ), toScene( mouse->pos() ) ).normalized(),
                    additive );
            }
            updateNodeColorsAndSelection();
            ui->pushButton_wireSection->setEnabled( !m_selection.empty() );
            statusBar()->showMessage(
                tr( "%1 node(s) selected." ).arg( static_cast< int >( m_selection.size() ) ) );
            return true;
        }
        return QMainWindow::eventFilter( watched, event );
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

    QSettings     settings;
    QString const defaultName = QString::fromStdString( m_model->GetName() ) + ".xmodel";
    QString const lastDir     = settings.value( "paths/saveDir" ).toString();
    QString const startPath =
        lastDir.isEmpty() ? defaultName : QDir( lastDir ).filePath( defaultName );

    QString fileName = QFileDialog::getSaveFileName(
        this, tr( "Export xModel" ), startPath,
        tr( "xLights model (*.xmodel);;All files (*)" ) );
    if( fileName.isEmpty() ) {
        return;  // user cancelled
    }

    settings.setValue( "paths/saveDir", QFileInfo( fileName ).absolutePath() );

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

AutoWire::Strategy MainWindow::selectedStrategy() const
{
    // Combo index 1 == Warnsdorff (see res/mainwindow.ui); 0/anything else == nearest.
    return ui->comboBox_wireStrategy->currentIndex() == 1 ? AutoWire::Strategy::Warnsdorff
                                                          : AutoWire::Strategy::NearestFirst;
}

std::vector< int > MainWindow::runSearch( Model& model, double wireGapMm, double startX,
                                          double startY, AutoWire::Strategy strategy )
{
    int const totalNodes = static_cast< int >( model.GetNodeCount() );

    // The search can run for millions of backtracking steps, which would freeze the
    // UI. Show a modal progress dialog with a Cancel button and let AutoWire pump it
    // via its progress callback; cancelling keeps the best partial path found so far.
    QProgressDialog progress( tr( "Auto-wiring nodes..." ), tr( "Cancel" ), 0, totalNodes, this );
    progress.setWindowTitle( tr( "AutoWire" ) );
    progress.setWindowModality( Qt::WindowModal );
    progress.setMinimumDuration( 250 );  // don't flash for instant solves

    // Capture the cancel via the signal into a flag rather than polling
    // wasCanceled(): the signal fires the moment the button is clicked, and the
    // flag can't be cleared by setValue()/auto-reset side effects mid-search.
    bool canceled = false;
    connect( &progress, &QProgressDialog::canceled, this, [ &canceled ]() { canceled = true; } );

    AutoWire autoWire( &model, wireGapMm );
    autoWire.SetStrategy( strategy );
    autoWire.SetProgressCallback(
        [ &progress, &canceled ]( int bestLen, int total, long long steps ) -> bool {
            if( progress.maximum() != total ) {
                progress.setMaximum( total );
            }
            progress.setValue( bestLen );
            progress.setLabelText(
                MainWindow::tr( "Wired %1 of %2 nodes (%3 steps)..." )
                    .arg( bestLen )
                    .arg( total )
                    .arg( steps ) );
            // Pump the event loop so the Cancel click is delivered and serviced even
            // while the longest-path count sits on a plateau.
            QCoreApplication::processEvents();
            return !canceled;
        } );

    autoWire.WireModel( startX, startY );
    progress.reset();
    return autoWire.GetIndexes();
}

int MainWindow::wireFrom( int startIndex, double wireGapMm, AutoWire::Strategy strategy )
{
    Node const& start = m_model->GetNodes().at( startIndex );

    spdlog::info( "AutoWire: gap {}mm, start ({}, {}), strategy {}", wireGapMm, start.X, start.Y,
                  strategy == AutoWire::Strategy::Warnsdorff ? "Warnsdorff" : "NearestFirst" );

    std::vector< int > const order =
        runSearch( *m_model, wireGapMm, start.X, start.Y, strategy );

    // Whole-model Auto Wire replaces any existing wiring and numbers 1..N.
    m_model->ClearWiring();
    int nodeNumber = 1;
    for( int index : order ) {
        m_model->SetNodeNumber( index, nodeNumber++ );
    }

    m_startNodeIndex = startIndex;
    m_selection.clear();
    refreshModelView();

    int const wiredCount = static_cast< int >( order.size() );
    spdlog::info( "AutoWire wired {}/{} nodes", wiredCount, m_model->GetNodeCount() );
    return wiredCount;
}

int MainWindow::nextNodeNumber() const
{
    int maxNum = 0;
    if( m_model ) {
        for( Node const& n : m_model->GetNodes() ) {
            maxNum = std::max( maxNum, n.NodeNumber );
        }
    }
    return maxNum + 1;
}

void MainWindow::manualAddNode( int idx )
{
    if( !m_model || idx < 0 || idx >= static_cast< int >( m_model->GetNodeCount() ) ) {
        return;
    }
    Node& n = m_model->GetEditNodes().at( idx );
    if( n.IsWired() ) {
        return;  // already part of the run
    }
    n.NodeNumber = nextNodeNumber();
    m_strokeAdded.insert( idx );
    refreshModelView( false );  // redraw labels/colours, keep the current zoom/pan
    statusBar()->showMessage( tr( "Wired node %1 (manual)." ).arg( n.NodeNumber ) );
}

void MainWindow::undoLastWire()
{
    if( !m_model ) {
        return;
    }
    std::vector< Node >& nodes = m_model->GetEditNodes();
    int maxNum = 0, idx = -1;
    for( int i = 0; i < static_cast< int >( nodes.size() ); ++i ) {
        if( nodes[ i ].NodeNumber > maxNum ) {
            maxNum = nodes[ i ].NodeNumber;
            idx    = i;
        }
    }
    if( idx < 0 ) {
        statusBar()->showMessage( tr( "Nothing to undo." ) );
        return;
    }
    nodes[ idx ].NodeNumber = 0;
    m_strokeAdded.erase( idx );
    refreshModelView( false );
    statusBar()->showMessage( tr( "Removed node %1." ).arg( maxNum ) );
}

void MainWindow::applyRectSelection( QRectF const& sceneRect, bool additive )
{
    if( !m_model ) {
        return;
    }
    if( !additive ) {
        m_selection.clear();
    }
    std::vector< Node > const& nodes = m_model->GetNodes();
    for( int i = 0; i < static_cast< int >( nodes.size() ); ++i ) {
        // Markers are drawn at (X, -Y) in scene space.
        if( sceneRect.contains( nodes[ i ].X, -nodes[ i ].Y ) ) {
            m_selection.insert( i );
        }
    }
}

void MainWindow::wireSection()
{
    if( !m_model || m_selection.empty() ) {
        return;
    }

    std::vector< Node > const& nodes = m_model->GetNodes();

    // Wire only the still-unwired nodes in the selection.
    std::vector< int > sel;
    for( int idx : m_selection ) {
        if( idx >= 0 && idx < static_cast< int >( nodes.size() ) && !nodes[ idx ].IsWired() ) {
            sel.push_back( idx );
        }
    }
    if( sel.empty() ) {
        QMessageBox::information( this, tr( "Wire Section" ),
                                 tr( "All selected nodes are already wired." ) );
        return;
    }

    // Pick the section start: the selected node nearest the last-wired node, so the
    // new run continues smoothly from existing wiring; otherwise the first selected.
    int startSel = sel.front();
    int lastNum = 0, lastIdx = -1;
    for( int i = 0; i < static_cast< int >( nodes.size() ); ++i ) {
        if( nodes[ i ].NodeNumber > lastNum ) {
            lastNum = nodes[ i ].NodeNumber;
            lastIdx = i;
        }
    }
    if( lastIdx >= 0 ) {
        double best = -1.0;
        for( int idx : sel ) {
            double const dx = nodes[ idx ].X - nodes[ lastIdx ].X;
            double const dy = nodes[ idx ].Y - nodes[ lastIdx ].Y;
            double const d  = dx * dx + dy * dy;
            if( best < 0.0 || d < best ) {
                best     = d;
                startSel = idx;
            }
        }
    }

    // Build a temporary model of just the selected nodes and remember the mapping
    // back to real indices, so AutoWire can run over the subset in isolation.
    Model              sub;
    std::vector< int > realIndex;  // temp index -> real model index
    realIndex.reserve( sel.size() );
    for( int idx : sel ) {
        sub.AddNode( nodes[ idx ] );
        realIndex.push_back( idx );
    }

    double const wireGapMm  = ui->spinBox_wireSize->value();
    Node const&  startNode  = nodes[ startSel ];
    std::vector< int > const order =
        runSearch( sub, wireGapMm, startNode.X, startNode.Y, selectedStrategy() );

    // Number the section on from the highest existing wire number.
    int num = nextNodeNumber();
    for( int subIdx : order ) {
        if( subIdx >= 0 && subIdx < static_cast< int >( realIndex.size() ) ) {
            m_model->SetNodeNumber( realIndex[ subIdx ], num++ );
        }
    }

    int const wired = static_cast< int >( order.size() );
    spdlog::info( "Wire Section: {}/{} selected nodes wired", wired, sel.size() );

    m_selection.clear();
    ui->pushButton_wireSection->setEnabled( false );
    refreshModelView( false );

    if( wired == static_cast< int >( sel.size() ) ) {
        statusBar()->showMessage( tr( "Wired section of %1 node(s)." ).arg( wired ) );
    } else {
        QMessageBox::warning(
            this, tr( "Wire Section" ),
            tr( "Only %1 of %2 selected nodes could be wired at this gap.\n"
                "Increase the wire gap, or try the Warnsdorff method." )
                .arg( wired )
                .arg( static_cast< int >( sel.size() ) ) );
    }
}

void MainWindow::on_pushButton_wireSection_clicked()
{
    wireSection();
}

void MainWindow::on_pushButton_undoWire_clicked()
{
    undoLastWire();
}

void MainWindow::on_comboBox_interactMode_currentIndexChanged( int index )
{
    switch( index ) {
        case 1:  m_mode = InteractMode::Manual;  break;
        case 2:  m_mode = InteractMode::Section; break;
        default: m_mode = InteractMode::PickStart; break;
    }

    // Reset transient interaction state when switching modes.
    m_dragging = false;
    m_strokeAdded.clear();
    if( m_selRect && m_scene ) {
        m_scene->removeItem( m_selRect );
        delete m_selRect;
    }
    m_selRect = nullptr;

    if( m_mode != InteractMode::Section ) {
        m_selection.clear();
        ui->pushButton_wireSection->setEnabled( false );
    } else {
        ui->pushButton_wireSection->setEnabled( !m_selection.empty() );
    }

    if( !m_nodeItems.empty() ) {
        updateNodeColorsAndSelection();
    }

    char const* const names[] = { "Pick start", "Manual wire", "Select section" };
    statusBar()->showMessage( tr( "%1 mode." ).arg( names[ std::clamp( index, 0, 2 ) ] ) );
}

void MainWindow::autoWireFromFirst( double wireGapMm )
{
    if( m_model && m_model->GetNodeCount() > 0 ) {
        wireFrom( 0, wireGapMm, selectedStrategy() );
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

    AutoWire::Strategy const strategy = selectedStrategy();
    int const wiredCount = wireFrom( m_startNodeIndex, wireGapMm, strategy );
    int const totalCount = static_cast< int >( m_model->GetNodeCount() );

    if( wiredCount == totalCount ) {
        QMessageBox::information( this, tr( "AutoWire" ),
                                 tr( "Wired all %1 nodes." ).arg( totalCount ) );
    } else if( strategy == AutoWire::Strategy::NearestFirst ) {
        // Nearest-first can stall in a greedy trap even when a complete path exists.
        // Point the user at the Warnsdorff method, which completes from almost any start.
        QMessageBox::warning(
            this, tr( "AutoWire" ),
            tr( "Nearest-first only wired %1 of %2 nodes before giving up.\n\n"
                "It can get stuck from some start nodes even when a full path exists. "
                "Try switching Method to \"Warnsdorff\", increasing the wire gap, or "
                "choosing a different start node." )
                .arg( wiredCount )
                .arg( totalCount ) );
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
