#pragma once

#include "AutoWire.h"
#include "Model.h"
#include "dxf/dxf_data.h"
#include "svg/svg_reader.h"

#include <QMainWindow>

#include <memory>
#include <set>
#include <vector>

namespace Ui {
class MainWindow;
}

class QEvent;
class QGraphicsScene;
class QGraphicsEllipseItem;
class QGraphicsRectItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Read a DXF file, build the model from it, and refresh the views.
    void loadDxf( QString const& fileName );

    // Read an SVG file: collect circle-like elements as hole candidates.
    void loadSvg( QString const& fileName );

    // Set the target hole diameter (mm) shown in the spin box.
    void setHoleDiameter( double mm );

    // Wire the loaded model starting from the first node (for CLI/testing).
    void autoWireFromFirst( double wireGapMm );

    // Export the current model to an .xmodel file (for CLI/testing).
    void exportModelTo( QString const& fileName );

public Q_SLOTS:

    void on_actionOpen_DXF_triggered();
	void on_actionOpen_SVG_triggered();
	void on_actionExport_xModel_triggered();
	void on_actionExit_triggered();
	void on_actionAutoWire_triggered();
	void on_pushButton_autoWire_clicked();
	void on_pushButton_wireSection_clicked();
	void on_pushButton_undoWire_clicked();
	void on_comboBox_interactMode_currentIndexChanged( int index );
	void on_comboBox_units_currentIndexChanged( int index );
	void on_doubleSpinBox_holeDia_editingFinished();
	void on_actionView_Logs_triggered();

protected:
    // Map clicks/drags in the drawing view to start-node selection.
    bool eventFilter( QObject* watched, QEvent* event ) override;

private:
    // Detect holes in the loaded DXF at the current target diameter and rebuild
    // the model from them.
    void detectHoles();

    // The DXF units code to use for conversions: the file's $INSUNITS, unless the
    // Units combo overrides it (e.g. for unitless files actually drawn in inches).
    int effectiveInsUnits() const;

    // Convert a real-world millimetre size into the loaded file's drawing units.
    double mmToDrawingUnits( double mm ) const;

    // Redraw the loaded model: the node list and the geometry in the view. Pass
    // fitView=false to keep the current zoom/pan (for incremental wiring updates).
    void refreshModelView( bool fitView = true );

    // Index of the model node nearest the given scene point, or -1.
    int nearestNodeIndex( double sceneX, double sceneY ) const;

    // Radius used to draw node markers, in drawing units.
    double nodeDisplayRadius() const;

    // Wire the model from the selected start node with the given gap (millimetres).
    void runAutoWire( double wireGapMm );

    // The wiring strategy currently chosen in the UI combo box.
    AutoWire::Strategy selectedStrategy() const;

    // Core wiring: wire the whole model from startIndex with the given gap
    // (clears any prior wiring and numbers 1..N); returns nodes wired.
    int wireFrom( int startIndex, double wireGapMm, AutoWire::Strategy strategy );

    // Run AutoWire (with the cancelable progress dialog) over `model` from the node
    // nearest (startX,startY); returns the discovered visiting order (node indices).
    std::vector< int > runSearch( Model& model, double wireGapMm, double startX,
                                  double startY, AutoWire::Strategy strategy );

    // --- manual + section wiring -----------------------------------------------
    enum class InteractMode { PickStart, Manual, Section };

    // Next 1-based wire number to assign (highest existing + 1), so manual picks and
    // wired sections chain into one continuous run.
    int nextNodeNumber() const;

    // Append node `idx` to the wire run (manual mode), if not already wired.
    void manualAddNode( int idx );

    // Clear the highest-numbered node (undo the last manual/section step).
    void undoLastWire();

    // Auto-wire the current section selection, numbering on from the highest.
    void wireSection();

    // Replace/extend the section selection with the nodes inside `sceneRect`.
    void applyRectSelection( QRectF const& sceneRect, bool additive );

    // Recolour markers for the current wired/selected/start state.
    void updateNodeColorsAndSelection();

    Ui::MainWindow *ui;

    std::unique_ptr<dxf_data> m_dxf_data;
    std::vector< svg_reader::Circle > m_svgCircles;  // active when loaded from SVG
    std::unique_ptr< Model > m_model;
    std::unique_ptr< QGraphicsScene > m_scene;

    std::string m_modelName;          // base name of the loaded DXF
    double      m_nodeRadius{ 0.0 };  // marker radius (drawing units)

    // One ellipse marker per model node (parallel to Model::GetNodes()).
    std::vector< QGraphicsEllipseItem* > m_nodeItems;
    int m_startNodeIndex{ -1 };

    // Interaction state for manual/section wiring.
    InteractMode       m_mode{ InteractMode::PickStart };
    std::set< int >    m_selection;             // section-mode selected node indices
    std::set< int >    m_strokeAdded;           // nodes added during the current manual drag
    bool               m_dragging{ false };     // a press-drag is in progress
    QPoint             m_pressViewPos;          // viewport pos where the press began
    QGraphicsRectItem* m_selRect{ nullptr };    // live rubber-band (owned by the scene)
};


