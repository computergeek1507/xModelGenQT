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
    void addVertex(const DL_VertexData& data) override;
    void addText(const DL_TextData& data) override;

    void addBlock(const DL_BlockData& data) override;
    void endBlock() override;
    void addInsert(const DL_InsertData& data) override;

    void setVariableInt(const std::string& key, int value, int code) override;

    std::unique_ptr<dxf_data> moveData() { return std::move(m_data); }

private:
    // Geometry goes to the current block while a block is being defined,
    // otherwise to model space.
    dxf_data::Geometry& target();

    std::unique_ptr<dxf_data> m_data;
    dxf_data::Block* m_currentBlock{ nullptr };

};
