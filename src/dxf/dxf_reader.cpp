#include "dxf_reader.h"

#include "..\dxflib\src\dl_dxf.h"

bool dxf_reader::openFile(const std::string& fileName){
    m_data = std::make_unique<dxf_data>();
    DL_Dxf dxf;
    bool success = dxf.in(fileName, this);
    if (success) {
        m_data->filename = fileName;
	}
    return success;
}

void dxf_reader::addPoint(const DL_PointData& data) {
    m_data->points.push_back(data);
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
    m_data->ellipses.push_back(data);
}

void dxf_reader::addPolyline(const DL_PolylineData& data) {
    //DL_Attributes attrib = getAttributes();
    //m_data->polylines.push_back(data);
    m_curr_entity.clear();
    if (data.number >3)
    m_curr_entity.m_isPolyLine = true;
	m_curr_entity.m_EntityFlag = data.flags;
}

void dxf_reader::addText(const DL_TextData& data) {
    m_data->texts.push_back(data);
}

void dxf_reader::addVertex(const DL_VertexData& aData)
{

    const DL_VertexData* vertex = &aData;

    if (m_curr_entity.m_first)
    {
        m_curr_entity.m_PolylineStart = aData;
            m_curr_entity.m_first = false;
        return;
    }

    m_curr_entity.m_LastCoordinate = aData;
}


void dxf_reader::endEntity()
{
    if (m_curr_entity.m_isPolyLine)
    {
        // Polyline flags bit 0 indicates closed (1) or open (0) polyline
        if (m_curr_entity.m_EntityFlag & 1)
        {
            
            if (std::abs(m_curr_entity.m_LastCoordinate.bulge) > 0.1 &&
                std::abs(m_curr_entity.m_LastCoordinate.bulge) < 1.0) {
                m_data->vertexs.push_back(m_curr_entity.m_LastCoordinate);
            }
            else {
                DL_LineData line(m_curr_entity.m_LastCoordinate.x, m_curr_entity.m_LastCoordinate.y, m_curr_entity.m_LastCoordinate.z,
                    m_curr_entity.m_PolylineStart.x, m_curr_entity.m_PolylineStart.y, m_curr_entity.m_PolylineStart.z);
                m_data->lines.push_back(line);
			}
        }        
    }

    m_curr_entity.clear();
}
