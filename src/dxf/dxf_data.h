#pragma once
#include "..\dxflib\src\dl_entities.h"

#include <string>
#include <vector>

//container class to store dxf data.
struct dxf_data {

    std::vector<DL_LineData> lines;
    std::vector<DL_CircleData> circles;
    std::vector<DL_ArcData> arcs;
    std::vector<DL_TextData> texts;
    std::vector<DL_EllipseData> ellipses;
    std::vector<DL_PolylineData> polylines;
    std::vector < DL_VertexData> vertexs;
    std::vector<DL_PointData> points;
	std::string filename;
};

