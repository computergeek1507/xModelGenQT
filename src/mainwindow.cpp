#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "AutoWire.h"
#include "dxf/dxf_reader.h"
#include "dxf/dxf_units.h"

#include <QBrush>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QInputDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QPen>

#include <spdlog/spdlog.h>

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
    spdlog::debug( "DXF entities: {} circles, {} points, {} lines (units code {})",
                   m_dxf_data->circles.size(), m_dxf_data->points.size(),
                   m_dxf_data->lines.size(), m_dxf_data->insUnits );

    // Build the model: each circle centre and each point becomes a node.
    auto model = std::make_unique< Model >();
    model->SetName( QFileInfo( fileName ).baseName().toStdString() );

    auto addNodeAt = [ & ]( double x, double y ) {
        model->AddNode( Node( static_cast< int >( std::lround( x ) ),
                              static_cast< int >( std::lround( y ) ) ) );
    };

    for( auto const& circle : m_dxf_data->circles ) {
        addNodeAt( circle.cx, circle.cy );
    }
    for( auto const& point : m_dxf_data->points ) {
        addNodeAt( point.x, point.y );
    }

    if( model->GetNodeCount() == 0 ) {
        spdlog::warn( "No nodes (circles/points) found in DXF: {}", fileName.toStdString() );
        QMessageBox::warning(
            this, tr( "Open DXF" ),
            tr( "No nodes found in \"%1\". Nodes are read from circles and points." )
                .arg( fileName ) );
        return;
    }

    m_model = std::move( model );
    spdlog::info( "Loaded {} nodes from {}", m_model->GetNodeCount(), fileName.toStdString() );

    refreshModelView();

    statusBar()->showMessage(
        tr( "Loaded %1 nodes from \"%2\"." )
            .arg( static_cast< int >( m_model->GetNodeCount() ) )
            .arg( QFileInfo( fileName ).fileName() ) );
}

void MainWindow::refreshModelView()
{
    ui->listWidgetNodes->clear();

    if( !m_scene ) {
        m_scene = std::make_unique< QGraphicsScene >();
        ui->graphicsViewDraw->setScene( m_scene.get() );
    }
    m_scene->clear();

    if( !m_model ) {
        return;
    }

    // Node list (shows wiring order once the model has been auto-wired).
    for( Node const& node : m_model->GetNodes() ) {
        ui->listWidgetNodes->addItem( QString::fromStdString( node.GetText() ) );
    }
    spdlog::info( "Node list populated with {} items", ui->listWidgetNodes->count() );

    // Draw the loaded geometry. DXF Y points up while scene Y points down, so
    // negate Y to keep the drawing the right way up.
    QPen const   pen( Qt::darkGray );
    QBrush const brush( Qt::yellow );

    if( m_dxf_data ) {
        for( auto const& circle : m_dxf_data->circles ) {
            m_scene->addEllipse( circle.cx - circle.radius,
                                 -circle.cy - circle.radius,
                                 circle.radius * 2.0, circle.radius * 2.0,
                                 pen, brush );
        }
        for( auto const& point : m_dxf_data->points ) {
            m_scene->addEllipse( point.x - 0.5, -point.y - 0.5, 1.0, 1.0, pen, brush );
        }
    }

    if( !m_scene->items().isEmpty() ) {
        QRectF const bounds = m_scene->itemsBoundingRect();
        m_scene->setSceneRect( bounds );
        ui->graphicsViewDraw->fitInView( bounds, Qt::KeepAspectRatio );
    }
}

void MainWindow::on_actionExport_xModel_triggered()
{
	
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

    // The drawing units of the loaded DXF ($INSUNITS); 0 if unknown.
    int const drawingUnits = m_dxf_data ? m_dxf_data->insUnits : dxf_units::Unitless;

    // Ask the user for the real-world gap: pick a unit, then enter the size.
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
        25.0, 0.0, std::numeric_limits< double >::max(), 3, &ok );
    if( !ok ) {
        return;  // user cancelled
    }

    // Convert the real-world gap into the model's drawing units.
    double wireGap = dxf_units::ToDrawingUnits( realWorldGap, realWorldUnit, drawingUnits );
    if( wireGap < 0.0 ) {
        // Drawing units are unknown: fall back to treating the model's coordinates
        // as millimeters so the conversion is still well-defined.
        wireGap = realWorldGap * dxf_units::MillimetersPerUnit( realWorldUnit );
        QMessageBox::information(
            this, tr( "AutoWire" ),
            tr( "The DXF file has no units set; assuming its coordinates are in millimeters." ) );
    }

    runAutoWire( wireGap );
}

void MainWindow::on_pushButton_autoWire_clicked()
{
    // The spin box gives the wire gap directly in the model's drawing units.
    runAutoWire( ui->spinBox_wireSize->value() );
}

void MainWindow::runAutoWire( double wireGapDrawingUnits )
{
    if( !m_model || m_model->GetNodeCount() == 0 ) {
        QMessageBox::information( this, tr( "AutoWire" ),
                                 tr( "There is no model loaded to wire. Open a DXF first." ) );
        return;
    }

    // Ask the user which node to start wiring from.
    std::vector< Node > const& nodes = m_model->GetNodes();
    QStringList                items;
    for( Node const& node : nodes ) {
        items << QStringLiteral( "%1, %2" ).arg( node.X ).arg( node.Y );
    }

    bool          ok     = false;
    QString const choice = QInputDialog::getItem( this, tr( "AutoWire" ),
                                                  tr( "Start node (X, Y):" ), items,
                                                  0, false, &ok );
    if( !ok ) {
        return;  // user cancelled
    }

    Node const& start = nodes.at( items.indexOf( choice ) );

    spdlog::info( "AutoWire: gap {} drawing units, start ({}, {})",
                  wireGapDrawingUnits, start.X, start.Y );

    AutoWire autoWire( m_model.get(), wireGapDrawingUnits );
    autoWire.WireModel( start.X, start.Y );

    // Write the discovered order back as 1-based node numbers.
    m_model->ClearWiring();
    int nodeNumber = 1;
    for( int index : autoWire.GetIndexes() ) {
        m_model->SetNodeNumber( index, nodeNumber++ );
    }

    refreshModelView();

    int const wiredCount = static_cast< int >( autoWire.GetIndexes().size() );
    int const totalCount = static_cast< int >( m_model->GetNodeCount() );

    spdlog::info( "AutoWire wired {}/{} nodes (complete: {})",
                  wiredCount, totalCount, autoWire.GetWorked() );

    if( autoWire.GetWorked() ) {
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
