#pragma once

#include "Model.h"
#include "dxf/dxf_data.h"

#include <QMainWindow>

#include <memory>
#include <vector>

namespace Ui {
class MainWindow;
}

class QEvent;
class QGraphicsScene;
class QGraphicsEllipseItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Read a DXF file, build the model from it, and refresh the views.
    void loadDxf( QString const& fileName );

    // Set the target hole diameter (mm) shown in the spin box.
    void setHoleDiameter( double mm );

    // Wire the loaded model starting from the first node (for CLI/testing).
    void autoWireFromFirst( double wireGapMm );

    // Export the current model to an .xmodel file (for CLI/testing).
    void exportModelTo( QString const& fileName );

public Q_SLOTS:

    void on_actionOpen_DXF_triggered();
	void on_actionExport_xModel_triggered();
	void on_actionExit_triggered();
	void on_actionAutoWire_triggered();
	void on_pushButton_autoWire_clicked();
	void on_doubleSpinBox_holeDia_editingFinished();
	void on_actionView_Logs_triggered();

protected:
    // Map clicks/drags in the drawing view to start-node selection.
    bool eventFilter( QObject* watched, QEvent* event ) override;

private:
    // Detect holes in the loaded DXF at the current target diameter and rebuild
    // the model from them.
    void detectHoles();

    // Convert a real-world millimetre size into the loaded file's drawing units.
    double mmToDrawingUnits( double mm ) const;

    // Redraw the loaded model: the node list and the geometry in the view.
    void refreshModelView();

    // Recolour the node markers for the current start/wired state.
    void updateNodeColors();

    // Index of the model node nearest the given scene point, or -1.
    int nearestNodeIndex( double sceneX, double sceneY ) const;

    // Radius used to draw node markers, in drawing units.
    double nodeDisplayRadius() const;

    // Wire the model from the selected start node with the given gap (millimetres).
    void runAutoWire( double wireGapMm );

    // Core wiring: wire from startIndex with the given gap; returns nodes wired.
    int wireFrom( int startIndex, double wireGapMm );

    Ui::MainWindow *ui;

    std::unique_ptr<dxf_data> m_dxf_data;
    std::unique_ptr< Model > m_model;
    std::unique_ptr< QGraphicsScene > m_scene;

    std::string m_modelName;          // base name of the loaded DXF
    double      m_nodeRadius{ 0.0 };  // marker radius (drawing units)

    // One ellipse marker per model node (parallel to Model::GetNodes()).
    std::vector< QGraphicsEllipseItem* > m_nodeItems;
    int m_startNodeIndex{ -1 };
};


