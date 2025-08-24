#pragma once
#include "dxf_data.h"

#include "..\dxflib\src\dl_creationadapter.h"
#include "..\dxflib\src\dl_entities.h"

#include <string>
#include <memory>

struct PolylineEntity
{
    bool m_isPolyLine{false};
    bool m_first{ true };
    int m_EntityFlag{0};           // a info flag to parse entities

    DL_VertexData m_LastCoordinate; 
    DL_VertexData m_PolylineStart;
    void clear() {
        m_isPolyLine = false;
        m_first = true;
        m_EntityFlag = 0;
    }
};

class dxf_reader : public DL_CreationAdapter {

public:
    dxf_reader(){ };
    ~dxf_reader(){ };

    bool openFile(const std::string& fileName);

    void addPoint(const DL_PointData& data) override;
    void addLine(const DL_LineData& data) override;
    void addArc(const DL_ArcData& data) override;
    void addCircle(const DL_CircleData& data) override;
    void addEllipse(const DL_EllipseData& data) override;
    void addPolyline(const DL_PolylineData& data) override;
    //void addPolyline(const DL_LwPolylineData& data) override;
    void addText(const DL_TextData& data) override;
    void addVertex(const DL_VertexData& aData) override;
    void endEntity() override;

    std::unique_ptr<dxf_data> moveData() { return std::move(m_data); }

    PolylineEntity m_curr_entity;

private:
    std::unique_ptr<dxf_data> m_data;

};
