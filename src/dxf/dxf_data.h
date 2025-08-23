#pragma once
#include "dl_entities.h"

//container class to store dxf data.
struct dxf_data {

    std::vector<DL_LineData>lines;
    std::vector<DL_CircleData>circles;
    std::vector<DL_ArcData>arcs;
    std::vector<DL_TextData>texts;
};

