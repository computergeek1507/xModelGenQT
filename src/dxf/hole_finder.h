#pragma once

#include "dxf_data.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

// Finds geometry that represents a round hole of a given diameter (e.g. a 12mm
// mounting hole), looking at circles, arcs, circle-like polylines, and loops of
// connected line segments. Block references (INSERTs) are expanded into world
// space first, so holes placed via blocks are found at their real positions.
//
// All sizes are in the DXF's drawing units; the caller converts the real-world
// hole size into drawing units before calling.
namespace hole_finder
{
    struct Hole {
        double x;  // centre X (drawing units)
        double y;  // centre Y (drawing units)
    };

    // A loop needs at least this many vertices to count as a circle (keeps
    // triangles/squares from being mistaken for holes).
    inline constexpr std::size_t kMinLoopVertices = 6;

    // Roundness: each vertex must sit within this fraction of the mean radius.
    // Loose enough for coarse line-segment approximations (e.g. ~20%).
    inline constexpr double kRoundnessFactor = 0.25;

    // Maximum block-nesting depth expanded, to bound runaway/recursive blocks.
    inline constexpr int kMaxInsertDepth = 24;

    namespace detail
    {
        constexpr double kPi = 3.14159265358979323846;

        // 2D affine transform: x' = a*x + c*y + e, y' = b*x + d*y + f.
        struct Affine {
            double a{ 1.0 }, b{ 0.0 }, c{ 0.0 }, d{ 1.0 }, e{ 0.0 }, f{ 0.0 };
        };

        inline std::pair<double, double> Apply( Affine const& t, double x, double y )
        {
            return { t.a * x + t.c * y + t.e, t.b * x + t.d * y + t.f };
        }

        inline double ScaleOf( Affine const& t )
        {
            return std::sqrt( std::abs( t.a * t.d - t.b * t.c ) );
        }

        // parent ∘ local (apply 'local' first, then 'parent').
        inline Affine Compose( Affine const& p, Affine const& l )
        {
            return {
                p.a * l.a + p.c * l.b, p.b * l.a + p.d * l.b,
                p.a * l.c + p.c * l.d, p.b * l.c + p.d * l.d,
                p.a * l.e + p.c * l.f + p.e, p.b * l.e + p.d * l.f + p.f,
            };
        }

        // Transform mapping a block-local point to its parent frame, for an insert
        // placed at (ix,iy) with the given scale/rotation and block base (bx,by).
        inline Affine InsertAffine( dxf_data::Insert const& ins, double bx, double by,
                                    double ix, double iy )
        {
            double const ang  = ins.angle * kPi / 180.0;
            double const cosA = std::cos( ang );
            double const sinA = std::sin( ang );

            Affine t;
            t.a = cosA * ins.sx;
            t.c = -sinA * ins.sy;
            t.b = sinA * ins.sx;
            t.d = cosA * ins.sy;
            t.e = ix - cosA * ins.sx * bx + sinA * ins.sy * by;
            t.f = iy - sinA * ins.sx * bx - cosA * ins.sy * by;
            return t;
        }

        // Append 'geo', transformed by 't', into 'out', expanding nested inserts.
        inline void FlattenInto( dxf_data const& data, dxf_data::Geometry const& geo,
                                 Affine const& t, int depth, dxf_data::Geometry& out )
        {
            double const scale = ScaleOf( t );

            for( auto const& circle : geo.circles ) {
                auto const [ x, y ] = Apply( t, circle.cx, circle.cy );
                DL_CircleData c = circle;
                c.cx            = x;
                c.cy            = y;
                c.radius        = circle.radius * scale;
                out.circles.push_back( c );
            }

            for( auto const& arc : geo.arcs ) {
                auto const [ x, y ] = Apply( t, arc.cx, arc.cy );
                DL_ArcData a = arc;
                a.cx         = x;
                a.cy         = y;
                a.radius     = arc.radius * scale;
                out.arcs.push_back( a );
            }

            for( auto const& pl : geo.polylines ) {
                dxf_data::PolyLine wpl;
                wpl.flags = pl.flags;
                wpl.vertices.reserve( pl.vertices.size() );
                for( auto const& v : pl.vertices ) {
                    auto const [ x, y ] = Apply( t, v.x, v.y );
                    DL_VertexData wv    = v;
                    wv.x                = x;
                    wv.y                = y;
                    wpl.vertices.push_back( wv );
                }
                out.polylines.push_back( std::move( wpl ) );
            }

            for( auto const& line : geo.lines ) {
                auto const [ x1, y1 ] = Apply( t, line.x1, line.y1 );
                auto const [ x2, y2 ] = Apply( t, line.x2, line.y2 );
                DL_LineData wl        = line;
                wl.x1                 = x1;
                wl.y1                 = y1;
                wl.x2                 = x2;
                wl.y2                 = y2;
                out.lines.push_back( wl );
            }

            if( depth >= kMaxInsertDepth ) {
                return;
            }

            for( auto const& ins : geo.inserts ) {
                auto const it = data.blocks.find( ins.blockName );
                if( it == data.blocks.end() ) {
                    continue;
                }
                dxf_data::Block const& block = it->second;

                int const cols = std::max( 1, ins.cols );
                int const rows = std::max( 1, ins.rows );
                double const ang  = ins.angle * kPi / 180.0;
                double const cosA = std::cos( ang );
                double const sinA = std::sin( ang );

                for( int r = 0; r < rows; ++r ) {
                    for( int c = 0; c < cols; ++c ) {
                        double const dx = c * ins.colSp;
                        double const dy = r * ins.rowSp;
                        double const ix = ins.x + cosA * dx - sinA * dy;
                        double const iy = ins.y + sinA * dx + cosA * dy;

                        Affine const local = InsertAffine( ins, block.bx, block.by, ix, iy );
                        FlattenInto( data, block, Compose( t, local ), depth + 1, out );
                    }
                }
            }
        }

        // Tests whether a loop of points is a round hole of the target size.
        inline bool LoopIsHole( std::vector<std::pair<double, double>> const& pts,
                                double targetRadius, double radiusTolerance,
                                double& outCx, double& outCy )
        {
            if( pts.size() < kMinLoopVertices ) {
                return false;
            }

            double sx = 0.0, sy = 0.0;
            for( auto const& p : pts ) {
                sx += p.first;
                sy += p.second;
            }
            double const cx = sx / static_cast<double>( pts.size() );
            double const cy = sy / static_cast<double>( pts.size() );

            double meanR = 0.0;
            for( auto const& p : pts ) {
                meanR += std::hypot( p.first - cx, p.second - cy );
            }
            meanR /= static_cast<double>( pts.size() );
            if( meanR <= 0.0 || std::abs( meanR - targetRadius ) > radiusTolerance ) {
                return false;
            }

            double const roundTol = meanR * kRoundnessFactor;
            for( auto const& p : pts ) {
                if( std::abs( std::hypot( p.first - cx, p.second - cy ) - meanR ) > roundTol ) {
                    return false;
                }
            }

            outCx = cx;
            outCy = cy;
            return true;
        }

        // Union-find over welded endpoints, used to group connected line segments.
        struct SegmentGraph {
            explicit SegmentGraph( double weldTolerance ) : weld( weldTolerance ) {}

            int vertexAt( double x, double y )
            {
                long long const gx = std::llround( x / weld );
                long long const gy = std::llround( y / weld );
                std::uint64_t const key =
                    ( static_cast<std::uint64_t>( static_cast<std::uint32_t>( gx ) ) << 32 )
                    | static_cast<std::uint32_t>( gy );

                auto const it = index.find( key );
                if( it != index.end() ) {
                    return it->second;
                }
                int const id = static_cast<int>( parent.size() );
                parent.push_back( id );
                coord.emplace_back( x, y );
                index.emplace( key, id );
                return id;
            }

            int find( int a )
            {
                while( parent[ a ] != a ) {
                    parent[ a ] = parent[ parent[ a ] ];
                    a            = parent[ a ];
                }
                return a;
            }

            void unite( int a, int b )
            {
                int const ra = find( a );
                int const rb = find( b );
                if( ra != rb ) {
                    parent[ ra ] = rb;
                }
            }

            double                                 weld;
            std::vector<int>                       parent;
            std::vector<std::pair<double, double>> coord;
            std::unordered_map<std::uint64_t, int> index;
        };
    }  // namespace detail

    // Expands all block references into a single world-space geometry set.
    inline dxf_data::Geometry Flatten( dxf_data const& data )
    {
        dxf_data::Geometry out;
        detail::FlattenInto( data, data.model, detail::Affine{}, 0, out );
        return out;
    }

    // Returns the centres of all holes whose radius is within 'radiusTolerance'
    // of 'diameter' / 2. Duplicates (e.g. a circle drawn as several arcs that
    // share a centre) are left for the caller's node de-duplication to collapse.
    inline std::vector<Hole> FindHoles( dxf_data const& data, double diameter,
                                        double radiusTolerance )
    {
        dxf_data::Geometry const geo = Flatten( data );

        double const targetRadius  = diameter / 2.0;
        auto         radiusMatches = [ & ]( double r ) {
            return std::abs( r - targetRadius ) <= radiusTolerance;
        };

        std::vector<Hole> holes;

        for( auto const& circle : geo.circles ) {
            if( radiusMatches( circle.radius ) ) {
                holes.push_back( { circle.cx, circle.cy } );
            }
        }

        for( auto const& arc : geo.arcs ) {
            if( radiusMatches( arc.radius ) ) {
                holes.push_back( { arc.cx, arc.cy } );
            }
        }

        for( auto const& pl : geo.polylines ) {
            std::vector<std::pair<double, double>> pts;
            pts.reserve( pl.vertices.size() );
            for( auto const& v : pl.vertices ) {
                pts.emplace_back( v.x, v.y );
            }
            double cx = 0.0, cy = 0.0;
            if( detail::LoopIsHole( pts, targetRadius, radiusTolerance, cx, cy ) ) {
                holes.push_back( { cx, cy } );
            }
        }

        // Loops formed by connected line segments (circles exploded into lines).
        if( !geo.lines.empty() ) {
            detail::SegmentGraph graph( std::max( diameter * 0.02, 1e-9 ) );
            for( auto const& line : geo.lines ) {
                graph.unite( graph.vertexAt( line.x1, line.y1 ),
                             graph.vertexAt( line.x2, line.y2 ) );
            }

            std::unordered_map<int, std::vector<std::pair<double, double>>> components;
            for( int i = 0; i < static_cast<int>( graph.parent.size() ); ++i ) {
                components[ graph.find( i ) ].push_back( graph.coord[ i ] );
            }

            for( auto const& [ root, pts ] : components ) {
                double cx = 0.0, cy = 0.0;
                if( detail::LoopIsHole( pts, targetRadius, radiusTolerance, cx, cy ) ) {
                    holes.push_back( { cx, cy } );
                }
            }
        }

        return holes;
    }
}
