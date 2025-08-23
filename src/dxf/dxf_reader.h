#pragma once
#include "dxf_data.h"

#include "dl_creationadapter.h"
#include "dl_entities.h"

#include <string>
#include <memory>

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
    void addText(const DL_TextData& data) override;

    std::unique_ptr<dxf_data> moveData() { return std::move(m_data); }

private:
    std::unique_ptr<dxf_data> m_data;

};
