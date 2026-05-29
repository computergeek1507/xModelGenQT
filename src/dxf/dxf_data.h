#pragma once
#include "dl_entities.h"

#include <string>
#include <unordered_map>
#include <vector>

//container class to store dxf data.
struct dxf_data {

    // A polyline (POLYLINE / LWPOLYLINE) and its vertices.
    struct PolyLine {
        std::vector<DL_VertexData> vertices;
        int flags{ 0 };  // DXF group 70; bit 0x1 set means the polyline is closed
        [[nodiscard]] bool IsClosed() const { return ( flags & 1 ) != 0; }
    };

    // A block reference (INSERT). Block geometry is placed at (x,y) with the given
    // scale/rotation; cols/rows describe a MINSERT array.
    struct Insert {
        std::string blockName;
        double x{ 0.0 }, y{ 0.0 };      // insertion point
        double sx{ 1.0 }, sy{ 1.0 };    // scale
        double angle{ 0.0 };            // rotation, degrees
        int    cols{ 1 }, rows{ 1 };
        double colSp{ 0.0 }, rowSp{ 0.0 };
    };

    // A collection of geometry: either model space or a block definition.
    struct Geometry {
        std::vector<DL_CircleData> circles;
        std::vector<DL_ArcData>    arcs;
        std::vector<PolyLine>      polylines;
        std::vector<DL_LineData>   lines;
        std::vector<Insert>        inserts;
    };

    // A named block definition with its insertion base point.
    struct Block : Geometry {
        std::string name;
        double bx{ 0.0 }, by{ 0.0 };  // base point
    };

    Geometry                               model;   // model-space geometry
    std::unordered_map<std::string, Block> blocks;  // block definitions by name

    // Kept for completeness; not used by hole detection.
    std::vector<DL_PointData> points;
    std::vector<DL_TextData>  texts;

    // Drawing units of the file, from the $INSUNITS header variable.
    // 0 = unitless/unknown, 1 = inches, 4 = millimeters, etc.
    int insUnits{ 0 };
};
