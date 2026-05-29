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
#include <limits>
#include <unordered_set>
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


bool Model::ExportModel( std::string const& filename )
{
    size_t const nodeCount = GetNodeCount();
    if( nodeCount == 0 ) {
        return false;
    }

    // Node coordinates are real-world positions (millimetres). Pick a grid pitch
    // equal to the typical spacing between holes (median nearest-neighbour
    // distance) so adjacent holes map to adjacent grid cells instead of producing
    // a huge millimetre-resolution grid.
    double pitch = 1.0;
    if( nodeCount > 1 ) {
        std::vector<double> nearest;
        nearest.reserve( nodeCount );
        for( size_t i = 0; i < nodeCount; ++i ) {
            double best = std::numeric_limits<double>::max();
            for( size_t j = 0; j < nodeCount; ++j ) {
                if( i == j ) {
                    continue;
                }
                double const d = model_utils::GetDistance( m_nodes[ i ], m_nodes[ j ] );
                if( d > 0.0 && d < best ) {
                    best = d;
                }
            }
            if( best < std::numeric_limits<double>::max() ) {
                nearest.push_back( best );
            }
        }
        if( !nearest.empty() ) {
            std::sort( nearest.begin(), nearest.end() );
            pitch = nearest[ nearest.size() / 2 ];  // median spacing
        }
    }
    if( !( pitch > 0.0 ) ) {
        pitch = 1.0;
    }

    double minx = m_nodes[ 0 ].X, maxx = m_nodes[ 0 ].X;
    double miny = m_nodes[ 0 ].Y, maxy = m_nodes[ 0 ].Y;
    for( auto const& n : m_nodes ) {
        minx = std::min( minx, n.X );
        maxx = std::max( maxx, n.X );
        miny = std::min( miny, n.Y );
        maxy = std::max( maxy, n.Y );
    }

    // Refine the pitch until every hole lands on its own grid cell (holes that are
    // slightly off a regular grid can otherwise collide and be lost), while keeping
    // the grid a sensible size.
    auto hasCollision = [ & ]( double p ) {
        std::unordered_set<long long> seen;
        for( auto const& n : m_nodes ) {
            long long const gx  = std::lround( ( n.X - minx ) / p );
            long long const gy  = std::lround( ( n.Y - miny ) / p );
            long long const key = ( gx << 32 ) ^ ( gy & 0xffffffffLL );
            if( !seen.insert( key ).second ) {
                return true;
            }
        }
        return false;
    };
    for( int attempt = 0; attempt < 12; ++attempt ) {
        long long const sx = std::lround( ( maxx - minx ) / pitch ) + 1;
        long long const sy = std::lround( ( maxy - miny ) / pitch ) + 1;
        if( !hasCollision( pitch ) || sx * sy > 4'000'000 ) {
            break;
        }
        pitch *= 0.5;  // finer grid
    }

    auto gridX = [ & ]( double x ) { return static_cast<int>( std::lround( ( x - minx ) / pitch ) ); };
    auto gridY = [ & ]( double y ) { return static_cast<int>( std::lround( ( y - miny ) / pitch ) ); };

    int const sizex = gridX( maxx ) + 1;
    int const sizey = gridY( maxy ) + 1;

    // -1 marks an empty cell; ToCustomModel renders those as blanks.
    std::vector<std::vector<int>> layer( sizey, std::vector<int>( sizex, -1 ) );
    for( size_t i = 0; i < nodeCount; ++i ) {
        int const gx  = gridX( m_nodes[ i ].X );
        int const gy  = gridY( m_nodes[ i ].Y );
        int const row = sizey - 1 - gy;  // xLights lists the top row first
        // Use the wiring order if the model has been auto-wired.
        int const value = m_nodes[ i ].NodeNumber > 0 ? m_nodes[ i ].NodeNumber
                                                       : static_cast<int>( i + 1 );
        if( row >= 0 && row < sizey && gx >= 0 && gx < sizex ) {
            layer[ row ][ gx ] = value;
        }
    }

    std::vector<std::vector<std::vector<int>>> data{ layer };

    std::ofstream f;
    f.open( filename.c_str(), std::ios::out );
    if( !f.good() ) {
        return false;
    }

    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<custommodel \n";
    f << "name=\"" << m_name << "\" ";
    f << "parm1=\"" << std::to_string( sizex ) << "\" ";
    f << "parm2=\"" << std::to_string( sizey ) << "\" ";
    f << "Depth=\"1\" ";
    f << "StringType=\"RGB Nodes\" ";
    f << "Transparency=\"0\" ";
    f << "PixelSize=\"2\" ";
    f << "ModelBrightness=\"\" ";
    f << "Antialias=\"1\" ";
    f << "StrandNames=\"\" ";
    f << "NodeNames=\"\" ";
    f << "CustomModel=\"";
    f << Model::ToCustomModel( data );
    f << "\" ";
    f << "SourceVersion=\"2025.8\" ";
    f << " >\n";
    f << "</custommodel>";

    f.close();
    return true;
}