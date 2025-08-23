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

void Model::SetBoundingBox( int minX, int maxX, int minY, int maxY )
{
    auto const width = ( maxX - minX ) + 1;
    auto const heigth = ( maxY - minY ) + 1;

    m_sizeX = width;
    m_sizeY = heigth;

    for( auto & node : m_nodes ) {
        auto scaleX = node.X - minX;
        auto scaleY = node.Y - minY;
        node.X = scaleX;
        node.Y = scaleY;
    }
}

void Model::ScaleNodesToGrid( int grid_width, int grid_heigth )
{
    int minX = INT_FAST32_MAX;
    int maxX = 0;
    int minY = INT_FAST32_MAX;
    int maxY = 0;

    for( auto const& node : m_nodes ) {
        minX = std::min(node.X, minX);
        maxX = std::max(node.X, maxX);
        minY = std::min(node.Y, minY);
        maxY = std::max(node.Y, maxY);
    }

    int width = std::abs(maxX - minX);
    int height = std::abs(maxY - minY);
    double scale_x = (double)width / grid_width;
    double scale_y = (double)width / grid_heigth;

    for( auto & node : m_nodes ) {
        int new_x  = (double)( node.X - minX ) / scale_x;
        int new_y  = (double)( node.Y - minY ) / scale_y;
        node.GridX = new_x;
        node.GridY = /*grid_heigth -*/ new_y;//screen Y is Top Most, so Flip?
    }
}

void Model::ExportModel( std::string const& filename, int grid_width, int grid_heigth ) 
{
    ScaleNodesToGrid(grid_width, grid_heigth);
    std::ofstream f;
    f.open( filename.c_str(), std::ios::out );
    if( !f.good() ) {
        return;
    }

    std::string cm;

    for( int x = 0; x <= grid_width + 1; x++ ) {
        for( int y = 0; y <= grid_heigth + 1; y++ ) {
            std::string cell;
            
            if( auto node = FindGridNode( y, x ); node ) {
                if( node->get().IsWired() ) {
                    cell = std::to_string( node->get().NodeNumber );
                } else {
                    cell = "1";
                }
            }
            cm += cell + ",";
        }
        cm += ";";
    }

    cm.pop_back();//remove last ";"

    f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<custommodel \n" ;
    f << "name=\"" << m_name << "\" ";
    f << "parm1=\"" << grid_width << "\" ";
    f << "parm2=\"" << grid_heigth << "\" ";
    f << "StringType=\"RGB Nodes\" ";
    f <<  "Transparency=\"0\" ";
    f <<  "PixelSize=\"2\" ";
    f <<  "ModelBrightness=\"\" ";
    f <<  "Antialias=\"1\" ";
    f <<  "StrandNames=\"\" ";
    f <<  "NodeNames=\"\" ";

    f <<  "CustomModel=\"";
    f <<  cm ;
    f <<  "\" ";
    f << "SourceVersion=\"2021.5\" ";
    f <<  " >\n";
    f <<  "</custommodel>";

    f.close();
}