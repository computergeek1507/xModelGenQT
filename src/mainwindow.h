#pragma once

#include "Model.h"
#include "dxf/dxf_data.h"

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

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
	
private:
    Ui::MainWindow *ui;

    std::unique_ptr<dxf_data> m_dxf_data;
    std::unique_ptr< Model > m_model;
};


