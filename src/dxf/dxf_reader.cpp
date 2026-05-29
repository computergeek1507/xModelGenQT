#include "dxf_reader.h"

#include "dl_dxf.h"

bool dxf_reader::openFile(const std::string& fileName){
    m_data = std::make_unique<dxf_data>();
    m_currentBlock = nullptr;
    DL_Dxf dxf;
    bool success = dxf.in(fileName, this);
    return success;
}

dxf_data::Geometry& dxf_reader::target() {
    return m_currentBlock ? static_cast<dxf_data::Geometry&>(*m_currentBlock)
                          : m_data->model;
}

void dxf_reader::addPoint(const DL_PointData& data) {
    m_data->points.push_back(data);
}

void dxf_reader::addLine(const DL_LineData& data) {
    target().lines.push_back(data);
}

void dxf_reader::addArc(const DL_ArcData& data) {
    target().arcs.push_back(data);
}

void dxf_reader::addCircle(const DL_CircleData& data) {
    target().circles.push_back(data);
}

void dxf_reader::addEllipse(const DL_EllipseData& data) {

}

void dxf_reader::addPolyline(const DL_PolylineData& data) {
    dxf_data::PolyLine pl;
    pl.flags = data.flags;
    pl.vertices.reserve(data.number);
    target().polylines.push_back(std::move(pl));
}

void dxf_reader::addVertex(const DL_VertexData& data) {
    // Vertices arrive after their owning polyline; append to the current one.
    auto& polylines = target().polylines;
    if (!polylines.empty()) {
        polylines.back().vertices.push_back(data);
    }
}

void dxf_reader::addText(const DL_TextData& data) {
    m_data->texts.push_back(data);
}

void dxf_reader::addBlock(const DL_BlockData& data) {
    dxf_data::Block block;
    block.name = data.name;
    block.bx = data.bpx;
    block.by = data.bpy;
    auto const result = m_data->blocks.emplace(data.name, std::move(block));
    m_currentBlock = &result.first->second;
}

void dxf_reader::endBlock() {
    m_currentBlock = nullptr;
}

void dxf_reader::addInsert(const DL_InsertData& data) {
    dxf_data::Insert ins;
    ins.blockName = data.name;
    ins.x = data.ipx;
    ins.y = data.ipy;
    ins.sx = data.sx;
    ins.sy = data.sy;
    ins.angle = data.angle;
    ins.cols = data.cols;
    ins.rows = data.rows;
    ins.colSp = data.colSp;
    ins.rowSp = data.rowSp;
    target().inserts.push_back(std::move(ins));
}

void dxf_reader::setVariableInt(const std::string& key, int value, int /*code*/) {
    if (key == "$INSUNITS") {
        m_data->insUnits = value;
    }
}
