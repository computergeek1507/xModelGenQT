#pragma once

#include "Model.h"
#include "dxf/dxf_data.h"

#include <QMainWindow>

#include <memory>

namespace Ui {
class MainWindow;
}

class QGraphicsScene;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Read a DXF file, build the model from it, and refresh the views.
    void loadDxf( QString const& fileName );

public Q_SLOTS:

    void on_actionOpen_DXF_triggered();
	void on_actionExport_xModel_triggered();
	void on_actionExit_triggered();
	void on_actionAutoWire_triggered();
	void on_pushButton_autoWire_clicked();
	void on_actionView_Logs_triggered();

private:
    // Redraw the loaded model: the node list and the geometry in the view.
    void refreshModelView();

    // Pick a start node and wire the model with the given gap (drawing units).
    void runAutoWire( double wireGapDrawingUnits );

    Ui::MainWindow *ui;

    std::unique_ptr<dxf_data> m_dxf_data;
    std::unique_ptr< Model > m_model;
    std::unique_ptr< QGraphicsScene > m_scene;
};


