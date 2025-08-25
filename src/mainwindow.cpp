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
#include <QProgressDialog>

#include <filesystem>
#include <utility>
#include <fstream>
#include <sstream>

enum class NodeColumns
{
    X = 0,
	Y,
	NodeNumber,
    LASTENTRY
};

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

    connect(m_boxScene, &BoxGraphicsScene::mousePosition, this, &MainWindow::updateMousePosition);
    connect(m_ui->boxView, &BoxGraphicsView::MouseSelectRectSignal, this, &MainWindow::updateSelectRect);
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
            statusBar()->showMessage(QString("%1 Nodes Found").arg(m_model->GetNodeCount()));
		}
		else {
            statusBar()->showMessage("Failed to Read DXF: " + openFileName);
		}
	}
}

void MainWindow::on_actionExport_xModel_triggered()
{
    if (!m_model) {
        return;
    }
    else
    {
        statusBar()->showMessage("No Model Loaded");
    }

    QString openFileName =
        QFileDialog::getSaveFileName(this, "Save xmodel File",
            m_model->GetName().c_str(),
            "xmodel File (*.xmodel)");

    if (!openFileName.isEmpty())
    {
        m_model->ExportModel(openFileName.toStdString());
    }
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionAutoWire_triggered()
{
    auto gap = m_ui->doubleSpinBox_wireSize->value();
    StartAutoWire(gap);
}

void MainWindow::on_actionView_Logs_triggered()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(m_appdir + "/log/"));
}

void MainWindow::on_pushButton_autoWire_clicked()
{
   on_actionAutoWire_triggered();
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
    //int minX = INT32_MAX;
    //int minY = INT32_MAX;
    //int maxX = 0;
    //int maxY = 0;
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

        //drawPoint(*m_boxScene, c.cx * 5.0, c.cy * 5.0, c.radius, QPen(Qt::black));
        //drawPoint( c.cx, c.cy, c.radius );
        //pixel hole are about 0.5 inches or 0.25
        if (0.2 > c.radius || c.radius > 0.3) {
            continue;
        }

        //int newX = (int)(c.cx * 2.0);
        //int newY = (int)(c.cy * 2.0);
        //
        //if (newX < minX) {
        //    minX = newX - 1;
        //}
        //if (newY < minY) {
        //    minY = newY - 1;
        //}
        //
        //if (newX > maxX) {
        //    maxX = newX + 1;
        //}
        //if (newY > maxY) {
        //    maxY = newY + 1;
        //}

        Node newNode(c.cx, c.cy, c.radius);
        m_model->AddNode( newNode );
    }
    for (auto const& v : m_dxf_data->vertexs) 
    {

        //drawPoint(*m_boxScene, v.x * 5.0, v.y * 5.0, std::abs(v.bulge)*5.0, QPen(Qt::black));
        //drawPoint( c.cx, c.cy, c.radius );
        //pixel hole are about 0.5 inches or 0.25
        //if (0.2 > c.radius || c.radius > 0.3) {
        //    continue;
        //}

        //int newX = (int)(v.x * 2.0);
        //int newY = (int)(v.y * 2.0);
        //
        //if (newX < minX) {
        //    minX = newX - 1;
        //}
        //if (newY < minY) {
        //    minY = newY - 1;
        //}
        //
        //if (newX > maxX) {
        //    maxX = newX + 1;
        //}
        //if (newY > maxY) {
        //    maxY = newY + 1;
        //}

        Node newNode(v.x, v.y, std::abs(v.bulge));
        m_model->AddNode(newNode);
    }

    for (auto const& l : m_dxf_data->lines) {
        //DrawLine( l.x1, l.y1, l.x2, l.y2 );
        //drawLine(*m_boxScene, l.x1, l.y1, l.x2, l.y2, QPen(Qt::black));
    }

    for (auto const& t : m_dxf_data->texts) {
        //dc.SetClippingRegion( pt, size );
        //dc.DrawText( t.text, t.height, t.xScaleFactor );
        //dc.DestroyClippingRegion();
    }

    //minX = std::max(minX, 0);
    //minY = std::max(minY, 0);
    //m_model->SetBoundingBox( minX, maxX, minY, maxY );
}

void MainWindow::StartAutoWire(double wireGap)
{
    if (!m_model) {
        return;
    }
    else
    {
        statusBar()->showMessage("No Model Loaded");
    }
    QProgressDialog progressDialog("Finding Nodes", "Cancel", 0, m_model->GetNodeCount(), this);
    progressDialog.setWindowModality(Qt::WindowModal);

    AutoWire sort(m_model.get(), wireGap);

	connect(&sort, &AutoWire::OnProgressSent, &progressDialog, &QProgressDialog::setValue);

    connect(&progressDialog, &QProgressDialog::canceled, [&sort]() {sort.Cancel();});

    if (auto node = m_model->FindNodeNumber(1); node) {
        m_model->ClearWiring();

        sort.WireModel2(node->get().X, node->get().Y);
        bool worked = sort.GetWorked();
        bool cal = sort.GetCancelled();

        if (worked || cal) {
            int order = 1;
            for (auto* index : sort.GetDoneNodes()) {
                index->NodeNumber = order;
                order++;
            }
            m_model->SortNodes();
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
    drawModel();
    m_ui->tableWidgetNodes->clearContents();
    m_ui->tableWidgetNodes->setRowCount(m_model->GetNodeCount());

    int row { 0 };
    for (auto const& node : m_model->GetNodes())
    {
        m_ui->tableWidgetNodes->setItem(row, (int)NodeColumns::X, new QTableWidgetItem());
        m_ui->tableWidgetNodes->item(row, (int)NodeColumns::X)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_ui->tableWidgetNodes->setItem(row, (int)NodeColumns::Y, new QTableWidgetItem());
        m_ui->tableWidgetNodes->item(row, (int)NodeColumns::Y)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        m_ui->tableWidgetNodes->setItem(row, (int)NodeColumns::NodeNumber, new QTableWidgetItem());
        m_ui->tableWidgetNodes->item(row, (int)NodeColumns::NodeNumber)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        m_ui->tableWidgetNodes->item(row, (int)NodeColumns::X)->setText(QString::number(node.X));
        m_ui->tableWidgetNodes->item(row, (int)NodeColumns::Y)->setText(QString::number(node.Y));
        m_ui->tableWidgetNodes->item(row, (int)NodeColumns::NodeNumber)->setText(QString::number(node.NodeNumber));
		row++;
    }
    m_ui->tableWidgetNodes->resizeColumnsToContents();
    //statusBar()->showMessage(QString("%1 Nodes Found").arg(m_model->GetNodeCount()));
    //StaticTextNodes->SetLabelText(wxString::Format("%zu Nodes Found", m_model->GetNodeCount()));
}

void MainWindow::UpdateRow(int row, Node const& _node)
{
    if (!m_model) {
        return;
    }
    if (row < 0 || row >= m_model->GetNodeCount()) {
        return;
    }
    m_ui->tableWidgetNodes->item(row, (int)NodeColumns::X)->setText(QString::number(_node.X));
    m_ui->tableWidgetNodes->item(row, (int)NodeColumns::Y)->setText(QString::number(_node.Y));
	m_ui->tableWidgetNodes->item(row, (int)NodeColumns::NodeNumber)->setText(QString::number(_node.NodeNumber));
}

void MainWindow::drawModel()
{
    if (!m_model) {
        return;
	}
    for (auto const& n : m_model->GetNodes())
    {
        drawPoint(*m_boxScene, n.X * 5.0, n.Y * 5.0, n.Radius * 5.0, n.IsFirst() ? Qt::red : n.IsWired() ? Qt::blue : Qt::black);

        if (n.IsWired())
        {
            drawTest(*m_boxScene, n.X * 5.0, n.Y * 5.0, QString::number(n.NodeNumber), Qt::black);
        }
    }
}

void MainWindow::drawPoint(QGraphicsScene& scene, double x, double y, double radius, QColor color)
{
    const QBrush blackbrush(color, Qt::SolidPattern );

    // y is negative due to graphics drawn from top left
    scene.addEllipse(x, -y - radius, radius, radius, Qt::NoPen, blackbrush);
}

void MainWindow::drawLine(QGraphicsScene& scene, double x1, double y1, double x2, double y2, QColor color)
{
    // y is negative due to graphics drawn from top left

    scene.addLine(x1, -y1, x2, -y2, QPen(color));

}

void MainWindow::drawTest(QGraphicsScene& scene, double x, double y, QString const& text, QColor color)
{
    const QPen mainPen(color, 1);
    const QBrush blackbrush(color, Qt::SolidPattern);

    QFont font;
    font.setPixelSize(int(2));
    font.setBold(false);
    font.setFamily("Calibri");


    QGraphicsTextItem* io = new QGraphicsTextItem;
    io->setPos(x, -y);
    io->setPlainText(text);
    io->setFont(font);
    //io->al
    //io->setTransformOriginPoint(((newText.Location2.x+10)*150),((newText.Location2.y+10)*150));
    //io->setRotation(newText.Angle*(-180.0/3.14159265359));
    //io->setRotation

    scene.addItem(io);
}

void MainWindow::updateSelectRect(QRect rect)
{
    //m_ui.pushButtonApply->setEnabled(true);

    QRect boundary = rect;//.normalized();

    QRect flippedRect((boundary.x()/5.0), (-boundary.y() / 5.0), (boundary.width() / 5.0), -(boundary.height() / 5.0));

	qDebug("Select Rect: X:%d, Y:%d, W:%d, H:%d", flippedRect.x(), flippedRect.y(), flippedRect.width(), flippedRect.height());

    //qDebug(flippedRect);

	//boundary.setBottom(-boundary.bottom());
	//boundary.setTop(-boundary.top());
    
    //rect = Settings::NormalizeBoundary(rect);

    //m_selection = rect;
    //drawRectangle(rect, QPen(Qt::red, 5));
    if (!m_model) {
        return;
    }

    int idx = m_model->FindNodeInBox(flippedRect);
    if (idx != -1) {

        if (auto node = m_model->GetNode(idx); node) {
            m_model->ClearWiring();
            m_model->SetNodeNumber(idx,1);
            UpdateRow(idx, node->get());
            m_ui->tableWidgetNodes->selectRow(idx);
            statusBar()->showMessage(QString("Node:%1, X:%2, Y:%3").arg(node->get().NodeNumber).arg(node->get().X).arg(node->get().Y));
            //drawPoint(*m_boxScene, node->get().X * 5.0, node->get().Y * 5.0, node->get().Radius * 5.0, node->get().IsFirst() ? Qt::red : node->get().IsWired() ? Qt::blue : Qt::black);
            drawModel();
        }
    }
    else {
        m_ui->tableWidgetNodes->clearSelection();
	}


	statusBar()->showMessage(QString("X:%1, Y:%2, W:%3, H:%4").arg(boundary.x()).arg(boundary.y()).arg(boundary.width()).arg(boundary.height()));
}

void MainWindow::updateMousePosition(qreal x, qreal y)
{
    //m_pcbX = (int)x;
    //m_pcbY = (int)y;
    QString const string = QString("X:%1, Y:%2").arg((int)x/5).arg((int)y/5);
    statusBar()->showMessage(string);
}
