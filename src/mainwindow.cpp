#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "config.h"

#include "spdlog/spdlog.h"

#include "spdlog/sinks/qt_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "BoxGraphicsScene.h"

#include "Model.h"
#include "AutoWire.h"

#include "dxf/dxf_reader.h"

#include <QMessageBox>
#include <QDesktopServices>
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QOperatingSystemVersion>

#include <filesystem>
#include <utility>
#include <fstream>
#include <sstream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    QCoreApplication::setApplicationName(PROJECT_NAME);
	QCoreApplication::setApplicationVersion(PROJECT_VER);
	m_ui->setupUi(this);

	auto const log_name{ "log.txt" };

	m_appdir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	std::filesystem::create_directory(m_appdir.toStdString());
	QString logdir = m_appdir + "/log/";
	std::filesystem::create_directory(logdir.toStdString());

	try
	{
		auto file{ std::string(logdir.toStdString() + log_name) };
		auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file, 1024 * 1024, 5, false);

		m_logger = std::make_shared<spdlog::logger>("box_design", rotating);
		m_logger->flush_on(spdlog::level::debug);
		m_logger->set_level(spdlog::level::debug);
		m_logger->set_pattern("[%D %H:%M:%S] [%L] %v");
		spdlog::register_logger(m_logger);
	}
	catch (std::exception& /*ex*/)
	{
		QMessageBox::warning(this, "Logger Failed", "Logger Failed To Start.");
	}

	setWindowTitle(windowTitle() + " v" + PROJECT_VER);

	m_settings = std::make_unique< QSettings>(m_appdir + "/settings.ini", QSettings::IniFormat);

	m_boxScene = new BoxGraphicsScene(this);
	//connect(m_boxScene, &BoxGraphicsScene::AddDevice, this, &MainWindow::OnAddDevice, Qt::QueuedConnection);

	//ui.graphicsView->setScene(&pageScene);
	m_ui->boxView->setScene(m_boxScene);// = new QGraphicsView(imageScene);

	m_ui->splitter->setStretchFactor(0, 1);
	m_ui->splitter->setStretchFactor(1, 5);

}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::on_actionOpen_dxf_triggered()
{
	QString openFileName =
		QFileDialog::getOpenFileName(this, "Open DXF File", 
			QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
			"DXF File (*.dxf)");

	if (!openFileName.isEmpty())
	{
		dxf_reader reader;
		if (reader.openFile(openFileName.toStdString())) {
			m_dxf_data = reader.moveData();
			Load_Dxf_Items();
            RefreshNodes();
		}
		else {
			//DisplayError("Failed to Read DXF: " + dlg.GetPath(), this);
		}
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
	
}

void MainWindow::on_actionView_Logs_triggered()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(m_appdir + "/log/"));
}

void MainWindow::on_MouseSelectRectSignal()
{

}

void MainWindow::Load_Dxf_Items()
{
    if (!m_dxf_data) {
        return;
    }

    std::filesystem::path p = m_dxf_data->filename;

    std::string name = p.filename().replace_extension().string();

    m_model = std::make_unique< Model >();

    m_model->SetName(name);
    int minX = INT32_MAX;
    int minY = INT32_MAX;
    int maxX = 0;
    int maxY = 0;
    /*
    foreach( LwPolyline entity in _doc.LwPolylines ) {
        if( !entity.IsClosed )
            continue;
        int count = entity.Vertexes.Count;
        if( count == 10 ) {
            double minNodeX = 10000000.0;
            double minNodeY = 10000000.0;
            double maxNodeX = 0.0;
            double maxNodeY = 0.0;

            foreach( LwPolylineVertex pt in entity.Vertexes ) {
                if( pt.Position.X < minNodeX )
                    minNodeX = pt.Position.X;
                if( pt.Position.Y < minNodeY )
                    minNodeY = pt.Position.Y;

                if( pt.Position.X > maxNodeX )
                    maxNodeX = pt.Position.X;
                if( pt.Position.Y > maxNodeY )
                    maxNodeY = pt.Position.Y;
            }

            double dist = maxNodeX - minNodeX;
            if( 0.4 > dist || dist > 0.7 )
                continue;

            double centerX = ( maxNodeX + minNodeX ) / 2.0;
            double centerY = ( maxNodeY + minNodeY ) / 2.0;

            int t = count;

            var newX = (int)( centerX * 2.0 );
            var newY = (int)( centerY * 2.0 );

            if( newX < minX )
                minX = newX - 1;
            if( newY < minY )
                minY = newY - 1;

            if( newX > maxX )
                maxX = newX + 1;
            if( newY > maxY )
                maxY = newY + 1;

            var newNode = new Node{
                GridX = newX,
                GridY = newY
            };
            _model.AddNode( newNode );
        }
    }
    */

    for (auto const& c : m_dxf_data->circles) {

        drawPoint(*m_boxScene, c.cx * 5.0, c.cy * 5.0, c.radius, QPen(Qt::black));
        //drawPoint( c.cx, c.cy, c.radius );
        //pixel hole are about 0.5 inches or 0.25
        if (0.2 > c.radius || c.radius > 0.3) {
            continue;
        }

        int newX = (int)(c.cx * 2.0);
        int newY = (int)(c.cy * 2.0);

        if (newX < minX) {
            minX = newX - 1;
        }
        if (newY < minY) {
            minY = newY - 1;
        }

        if (newX > maxX) {
            maxX = newX + 1;
        }
        if (newY > maxY) {
            maxY = newY + 1;
        }

        Node newNode(newX, newY );
        m_model->AddNode( newNode );
    }
    for (auto const& v : m_dxf_data->vertexs) 
    {

        drawPoint(*m_boxScene, v.x * 5.0, v.y * 5.0, std::abs(v.bulge)*5.0, QPen(Qt::black));
        //drawPoint( c.cx, c.cy, c.radius );
        //pixel hole are about 0.5 inches or 0.25
        //if (0.2 > c.radius || c.radius > 0.3) {
        //    continue;
        //}

        int newX = (int)(v.x * 2.0);
        int newY = (int)(v.y * 2.0);

        if (newX < minX) {
            minX = newX - 1;
        }
        if (newY < minY) {
            minY = newY - 1;
        }

        if (newX > maxX) {
            maxX = newX + 1;
        }
        if (newY > maxY) {
            maxY = newY + 1;
        }

        Node newNode(v.x, v.y);
        m_model->AddNode(newNode);
    }

    for (auto const& l : m_dxf_data->lines) {
        //DrawLine( l.x1, l.y1, l.x2, l.y2 );
        drawLine(*m_boxScene, l.x1, l.y1, l.x2, l.y2, QPen(Qt::black));
    }

    for (auto const& t : m_dxf_data->texts) {
        //dc.SetClippingRegion( pt, size );
        //dc.DrawText( t.text, t.height, t.xScaleFactor );
        //dc.DestroyClippingRegion();
    }

    minX = std::max(minX, 0);
    minY = std::max(minY, 0);
    //m_model->SetBoundingBox( minX, maxX, minY, maxY );
}

void MainWindow::StartAutoWire(int wireGap)
{
    AutoWire sort(m_model.get(), wireGap);

    if (auto node = m_model->FindNodeNumber(1); node) {
        m_model->ClearWiring();

        sort.WireModel(node->get().X, node->get().Y);
        bool worked = sort.GetWorked();

        if (worked) {
            int order = 1;
            for (int index : sort.GetIndexes()) {
                m_model->GetNode(index)->get().NodeNumber = order;
                order++;
            }

        }
        else {
            if (auto node2 = m_model->FindNode(node->get().X, node->get().Y); node2) {
                node2->get().NodeNumber = 1;
            }
        }
        statusBar()->showMessage(worked ? "Worked!" : "Didn't Work:(");
    }
    else {
        statusBar()->showMessage("No Starting Node Set");

    }
    //PanelPictureView->DrawPicture();
    RefreshNodes();
}

void MainWindow::RefreshNodes()
{
    m_ui->tableWidgetNodes->clearContents();
    m_ui->tableWidgetNodes->setRowCount(m_model->GetNodeCount());

    int row { 0 };
    for (auto const& node : m_model->GetNodes())
    {
        m_ui->tableWidgetNodes->setItem(row, 0, new QTableWidgetItem());
        m_ui->tableWidgetNodes->item(row, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_ui->tableWidgetNodes->setItem(row, 1, new QTableWidgetItem());
        m_ui->tableWidgetNodes->item(row, 1)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_ui->tableWidgetNodes->setItem(row, 2, new QTableWidgetItem());
        m_ui->tableWidgetNodes->item(row, 2)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        m_ui->tableWidgetNodes->item(row, 0)->setText(QString::number(node.X));
        m_ui->tableWidgetNodes->item(row, 1)->setText(QString::number(node.Y));
        m_ui->tableWidgetNodes->item(row, 2)->setText(QString::number(node.NodeNumber));
		row++;
    }
    m_ui->tableWidgetNodes->resizeColumnsToContents();
    statusBar()->showMessage(QString("%1 Nodes Found").arg(m_model->GetNodeCount()));
    //StaticTextNodes->SetLabelText(wxString::Format("%zu Nodes Found", m_model->GetNodeCount()));
}

void MainWindow::drawPoint(QGraphicsScene& scene, double x, double y, double radius, QPen pen)
{
    const QBrush blackbrush(Qt::black, Qt::SolidPattern );

    // y is negative due to graphics drawn from top left
    scene.addEllipse(x, -y - radius, radius, radius, Qt::NoPen, blackbrush);
}

void MainWindow::drawLine(QGraphicsScene& scene, double x1, double y1, double x2, double y2, QPen pen)
{
    // y is negative due to graphics drawn from top left

    scene.addLine(x1, -y1, x2, -y2, pen);

}
