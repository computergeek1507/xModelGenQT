#pragma once

#include "Model.h"
#include "dxf/dxf_data.h"

#include "spdlog/spdlog.h"
#include "spdlog/common.h"

#include <QMainWindow>
#include <QSettings>
#include <QGraphicsRectItem>
#include <QGraphicsView>

namespace Ui {
class MainWindow;
}

struct BoxGraphicsScene;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public Q_SLOTS:

    void on_actionOpen_dxf_triggered();
	void on_actionExport_xModel_triggered();
	void on_actionExit_triggered();
	void on_actionAutoWire_triggered();
	void on_actionView_Logs_triggered();

    void on_MouseSelectRectSignal();
	
private:
    Ui::MainWindow *m_ui;

    std::unique_ptr<dxf_data> m_dxf_data;
    std::unique_ptr< Model > m_model;

    BoxGraphicsScene* m_boxScene;
	
	std::shared_ptr<spdlog::logger> m_logger{ nullptr };
    std::unique_ptr<QSettings> m_settings{ nullptr };
    QString m_appdir;
    void Load_Dxf_Items();
    void StartAutoWire(int wireGap);
    void RefreshNodes();
    void drawPoint(QGraphicsScene& scene, double x, double y, double rad, QPen pen);
    void drawLine(QGraphicsScene& scene, double x1, double y1, double x2, double y2, QPen pen);
};


