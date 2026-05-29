/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/smeighan/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/smeighan/xLights/blob/master/License.txt
 **************************************************************/

#include "Model.h"

#include <algorithm>
#include <fstream>
#include "model_utils.h"

 void Model::AddNode( Node _node )
{
     if( std::find( m_nodes.begin(), m_nodes.end(), _node ) == m_nodes.end() ) {
        m_nodes.push_back( _node );
     }
 }

void Model::SortNodes()
{
     std::sort( std::begin( m_nodes ),
                std::end( m_nodes ),
                []( Node a, Node b ) { return a.NodeNumber > b.NodeNumber; } );
}

void Model::ClearWiring()
{
    for( auto& n : m_nodes ) {
        n.ClearWiring();
    }
}

void Model::ClearCloseNodes()
{
    for (auto& n : m_nodes) {
        n.ClearCloseNodes();
    }
}

void Model::FindCloseNodes(double distance)
{
    ClearCloseNodes();

    for (auto& n : m_nodes) {
        for (auto& m : m_nodes) {
            if (&n == &m) {
                continue;
            }
            double dist = model_utils::GetDistance(n, m);
            if (dist <= distance) {
                n.AddCloseNode(&m);
            }
		}
    }
}

int Model::FindNodeInBox(QRect rect)
    {
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (rect.contains(QPoint(m_nodes[i].X, m_nodes[i].Y))) {
            return i;
        }
    }
    return -1;
}


void Model::SetBoundingBox( int minX, int maxX, int minY, int maxY )
{
    auto const width = ( maxX - minX ) + 1;
    auto const heigth = ( maxY - minY ) + 1;

    //m_sizeX = width;
    //m_sizeY = heigth;

    for( auto & node : m_nodes ) {
        auto scaleX = node.X - minX;
        auto scaleY = node.Y - minY;
        node.X = scaleX;
        node.Y = scaleY;
    }
}

void Model::ScaleNodesToGrid( int grid_width, int grid_heigth )
{
    //int minX = INT_FAST32_MAX;
    //int maxX = 0;
    //int minY = INT_FAST32_MAX;
    //int maxY = 0;
    //
    //for( auto const& node : m_nodes ) {
    //    minX = std::min(node.X, minX);
    //    maxX = std::max(node.X, maxX);
    //    minY = std::min(node.Y, minY);
    //    maxY = std::max(node.Y, maxY);
    //}
    //
    //int width = std::abs(maxX - minX);
    //int height = std::abs(maxY - minY);
    //double scale_x = (double)width / grid_width;
    //double scale_y = (double)width / grid_heigth;
    //
    //for( auto & node : m_nodes ) {
    //    int new_x  = (double)( node.X - minX ) / scale_x;
    //    int new_y  = (double)( node.Y - minY ) / scale_y;
    //    //node.GridX = new_x;
    //    //node.GridY = /*grid_heigth -*/ new_y;//screen Y is Top Most, so Flip?
    //}
}

bool Model::FindCustomModelScale(int scale) const
{
    size_t nodeCount = GetNodeCount();
    if (nodeCount <= 1) {
        return true;
    }
    for (int i = 0; i < nodeCount; ++i) {
        for (int j = i + 1; j < nodeCount; ++j) {
            int x1 = (m_nodes[i].X * scale);
            int y1 = (m_nodes[i].Y * scale);
            int x2 = (m_nodes[i].X * scale);
            int y2 = (m_nodes[i].Y * scale);
            if (x1 == x2 && y1 == y2) {
                return false;
            }
        }
    }
    return true;
}

std::string Model::ToCustomModel(const std::vector<std::vector<std::vector<int>>>& model) {
    std::string customModel = "";
    for (int l = 0; l < model.size(); l++) {
        if (!customModel.empty()) {
            customModel += "|";
        }
        for (int r = 0; r < model[l].size(); r++) {
            if (r > 0) {
                customModel += ";";
            }
            for (int c = 0; c < model[l][r].size(); c++) {
                if (c > 0) {
                    customModel += ",";
                }
                if (model[l][r][c] >= 0) {
                    customModel += std::to_string(model[l][r][c]);
                }
            }
        }
    }
    return customModel;
}


void Model::ExportModel( std::string const& filename ) 
{
    float minsx = 99999;
    float minsy = 99999;
    float maxsx = -1;
    float maxsy = -1;

    size_t nodeCount = GetNodeCount();
    for (size_t i = 0; i < nodeCount; ++i) {
        float Sbufx = m_nodes[i].X;
        float Sbufy = m_nodes[i].Y;
        if (Sbufx < minsx)
            minsx = Sbufx;
        if (Sbufx > maxsx)
            maxsx = Sbufx;
        if (Sbufy < minsy)
            minsy = Sbufy;
        if (Sbufy > maxsy)
            maxsy = Sbufy;
    }
    int scale = 1;

    while (!FindCustomModelScale(scale)) {
        ++scale;
        if (scale > 100) { // I(Scott) am afraid of infinite while loops
            scale = 1;
            break;
        }
    }
    int minx = std::floor(minsx);
    int miny = std::floor(minsy);
    int maxx = std::ceil(maxsx);
    int maxy = std::ceil(maxsy);
    int sizex = maxx - minx + 1;
    int sizey = maxy - miny + 1;

    sizex *= scale;
    sizey *= scale;

    int* nodeLayout = (int*)malloc(sizey * sizex * sizeof(int));
    memset(nodeLayout, 0xFF, sizey * sizex * sizeof(int));

    for (int i = 0; i < nodeCount; ++i) {
        int x = (m_nodes[i].X - minx) * scale;
        int y = (sizey - ((m_nodes[i].Y - miny) * scale) - 1);
        nodeLayout[y * sizex + x] = i + 1;
    }

    std::vector<std::vector<std::vector<int>>> data;

    auto layer = std::vector<std::vector<int>>();
    for (int y = 0; y < sizey; ++y) {
        std::vector<int> row;
        for (int x = 0; x < sizex; ++x) {
            row.push_back(nodeLayout[y * sizex + x]);
        }
        layer.push_back(row);
    }
    data.push_back(layer);

    free(nodeLayout);

    //ScaleNodesToGrid(grid_width, grid_heigth);
    std::ofstream f;
    f.open( filename.c_str(), std::ios::out );
    if( !f.good() ) {
        return;
    }

    //std::string cm;
    //
    //for( int x = 0; x <= sizey + 1; x++ ) {
    //   for( int y = 0; y <= sizex + 1; y++ ) {
    //        std::string cell;
    //        
    //        //if( auto node = FindGridNode( y, x ); node ) {
    //        //    if( node->get().IsWired() ) {
    //        //        cell = std::to_string( node->get().NodeNumber );
    //        //    } else {
    //        //        cell = "1";
    //        //    }
    //        //}
    //        cm += cell + ",";
    //    }
    //    cm += ";";
    //}
    //
    //cm.pop_back();//remove last ";"

    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<custommodel \n" ;
    f << "name=\"" << m_name << "\" ";
    f << "parm1=\"" << std::to_string(sizex) << "\" ";
    f << "parm2=\"" << std::to_string(sizey) << "\" ";
    f << "Depth=\"1\" ";
    f << "StringType=\"RGB Nodes\" ";
    f <<  "Transparency=\"0\" ";
    f <<  "PixelSize=\"2\" ";
    f <<  "ModelBrightness=\"\" ";
    f <<  "Antialias=\"1\" ";
    f <<  "StrandNames=\"\" ";
    f <<  "NodeNames=\"\" ";

    f <<  "CustomModel=\"";
    //f <<  cm ;
    f << Model::ToCustomModel(data);
    f <<  "\" ";
    f << "SourceVersion=\"2025.8\" ";
    f <<  " >\n";
    f <<  "</custommodel>";

    f.close();
}