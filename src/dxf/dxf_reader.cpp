#include "dxf_reader.h"

#include "dl_dxf.h"

bool dxf_reader::openFile(const std::string& fileName){
    m_data = std::make_unique<dxf_data>();
    DL_Dxf dxf;
    bool success = dxf.in(fileName, this);
    return success;
}

void dxf_reader::addPoint(const DL_PointData& data) {
    
}

void dxf_reader::addLine(const DL_LineData& data) {
    DL_Attributes attrib = getAttributes();
    m_data->lines.push_back(data);
}

void dxf_reader::addArc(const DL_ArcData& data) {
    m_data->arcs.push_back(data);
}

void dxf_reader::addCircle(const DL_CircleData& data) {
    m_data->circles.push_back(data);
}

void dxf_reader::addEllipse(const DL_EllipseData& data) {
    
}

void dxf_reader::addPolyline(const DL_PolylineData& data) {
    
}

void dxf_reader::addText(const DL_TextData& data) {
    m_data->texts.push_back(data);
}