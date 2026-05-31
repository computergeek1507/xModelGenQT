#pragma once

#include <QFile>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>

#include <cmath>
#include <vector>

// Reads circle-like geometry from an SVG file as candidate holes. <circle> and
// near-circular <ellipse> elements are collected, element/group `transform`
// attributes are applied, and coordinates are scaled to millimetres using the
// root <svg> width + viewBox (falling back to 1 user unit = 1 mm).
//
// Returned positions use the app's node convention: X right, Y up (SVG Y points
// down, so it is negated). Diameters are in millimetres for matching against the
// target hole size, exactly like the DXF hole finder.
namespace svg_reader
{
    struct Circle {
        double x;    // mm, node convention (X right)
        double y;    // mm, node convention (Y up)
        double dia;  // mm
    };

    namespace detail
    {
        constexpr double kPi = 3.14159265358979323846;

        // 2D affine: x' = a*x + c*y + e, y' = b*x + d*y + f.
        struct Affine {
            double a{ 1.0 }, b{ 0.0 }, c{ 0.0 }, d{ 1.0 }, e{ 0.0 }, f{ 0.0 };
        };

        inline Affine Compose( Affine const& p, Affine const& l )
        {
            return {
                p.a * l.a + p.c * l.b, p.b * l.a + p.d * l.b,
                p.a * l.c + p.c * l.d, p.b * l.c + p.d * l.d,
                p.a * l.e + p.c * l.f + p.e, p.b * l.e + p.d * l.f + p.f,
            };
        }

        inline void Apply( Affine const& t, double x, double y, double& ox, double& oy )
        {
            ox = t.a * x + t.c * y + t.e;
            oy = t.b * x + t.d * y + t.f;
        }

        inline double ScaleOf( Affine const& t )
        {
            return std::sqrt( std::abs( t.a * t.d - t.b * t.c ) );
        }

        // Parse a numeric list out of "f1(a,b) f2(c)" argument text.
        inline std::vector< double > Numbers( QString const& s )
        {
            std::vector< double > out;
            QString               cur;
            auto flush = [ & ]() {
                if( !cur.isEmpty() ) {
                    bool ok = false;
                    double const v = cur.toDouble( &ok );
                    if( ok ) {
                        out.push_back( v );
                    }
                    cur.clear();
                }
            };
            for( QChar ch : s ) {
                if( ch.isDigit() || ch == '.' || ch == '-' || ch == '+' || ch == 'e'
                    || ch == 'E' ) {
                    cur.append( ch );
                } else {
                    flush();
                }
            }
            flush();
            return out;
        }

        // Parse an SVG transform attribute (translate/scale/rotate/matrix/skew*)
        // into a single affine, applied left-to-right as SVG specifies.
        inline Affine ParseTransform( QString const& spec )
        {
            Affine m;  // identity
            int    i = 0;
            int const n = spec.size();
            while( i < n ) {
                // read a function name
                while( i < n && !spec[ i ].isLetter() ) {
                    ++i;
                }
                int const nameStart = i;
                while( i < n && spec[ i ].isLetter() ) {
                    ++i;
                }
                if( i >= n || nameStart == i ) {
                    break;
                }
                QString const name = spec.mid( nameStart, i - nameStart );
                int const open = spec.indexOf( '(', i );
                if( open < 0 ) {
                    break;
                }
                int const close = spec.indexOf( ')', open );
                if( close < 0 ) {
                    break;
                }
                std::vector< double > const v = Numbers( spec.mid( open + 1, close - open - 1 ) );
                i = close + 1;

                Affine t;
                if( name == "translate" ) {
                    t.e = v.size() > 0 ? v[ 0 ] : 0.0;
                    t.f = v.size() > 1 ? v[ 1 ] : 0.0;
                } else if( name == "scale" ) {
                    t.a = v.size() > 0 ? v[ 0 ] : 1.0;
                    t.d = v.size() > 1 ? v[ 1 ] : t.a;
                } else if( name == "rotate" ) {
                    double const ang  = ( v.size() > 0 ? v[ 0 ] : 0.0 ) * kPi / 180.0;
                    double const cosA = std::cos( ang );
                    double const sinA = std::sin( ang );
                    Affine r{ cosA, sinA, -sinA, cosA, 0.0, 0.0 };
                    if( v.size() >= 3 ) {  // rotate about (cx,cy)
                        Affine const pre{ 1, 0, 0, 1, v[ 1 ], v[ 2 ] };
                        Affine const post{ 1, 0, 0, 1, -v[ 1 ], -v[ 2 ] };
                        t = Compose( Compose( pre, r ), post );
                    } else {
                        t = r;
                    }
                } else if( name == "matrix" && v.size() >= 6 ) {
                    t = Affine{ v[ 0 ], v[ 1 ], v[ 2 ], v[ 3 ], v[ 4 ], v[ 5 ] };
                } else if( name == "skewX" && v.size() >= 1 ) {
                    t.c = std::tan( v[ 0 ] * kPi / 180.0 );
                } else if( name == "skewY" && v.size() >= 1 ) {
                    t.b = std::tan( v[ 0 ] * kPi / 180.0 );
                }
                m = Compose( m, t );
            }
            return m;
        }

        // Convert an SVG length (e.g. "200mm", "8.5in", "300px", "300") to mm.
        // Returns -1 if it has no recognised physical unit and isn't a bare number.
        inline double LengthToMm( QString s )
        {
            s = s.trimmed();
            int split = s.size();
            for( int i = 0; i < s.size(); ++i ) {
                QChar const ch = s[ i ];
                if( !( ch.isDigit() || ch == '.' || ch == '-' || ch == '+' || ch == 'e'
                       || ch == 'E' ) ) {
                    split = i;
                    break;
                }
            }
            bool ok = false;
            double const num  = s.left( split ).toDouble( &ok );
            if( !ok ) {
                return -1.0;
            }
            QString const unit = s.mid( split ).trimmed().toLower();
            if( unit == "mm" ) return num;
            if( unit == "cm" ) return num * 10.0;
            if( unit == "in" ) return num * 25.4;
            if( unit == "pt" ) return num * 25.4 / 72.0;
            if( unit == "pc" ) return num * 25.4 / 6.0;
            if( unit == "px" || unit.isEmpty() ) return num * 25.4 / 96.0;  // CSS px
            return -1.0;  // %, em, ... : unsupported
        }
    }  // namespace detail

    inline std::vector< Circle > ReadCircles( QString const& path )
    {
        std::vector< Circle > circles;
        QFile                 file( path );
        if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
            return circles;
        }

        QXmlStreamReader      xml( &file );
        std::vector< detail::Affine > stack;  // composed transform per open element
        // Raw (user-unit) circles; scaled to mm after we know the viewBox/width.
        struct Raw { double x, y, r; };
        std::vector< Raw > raw;

        double mmPerUnit = 1.0;  // fall back to 1 user unit = 1 mm

        while( !xml.atEnd() ) {
            QXmlStreamReader::TokenType const tok = xml.readNext();
            if( tok == QXmlStreamReader::StartElement ) {
                QXmlStreamAttributes const attrs = xml.attributes();
                auto av = [ & ]( char const* n ) { return attrs.value( QLatin1String( n ) ); };
                auto has = [ & ]( char const* n ) {
                    return attrs.hasAttribute( QLatin1String( n ) );
                };

                detail::Affine elemT;
                if( has( "transform" ) ) {
                    elemT = detail::ParseTransform( av( "transform" ).toString() );
                }
                detail::Affine const cur =
                    stack.empty() ? elemT : detail::Compose( stack.back(), elemT );
                stack.push_back( cur );

                auto const name = xml.name();
                if( name == QLatin1String( "svg" ) ) {
                    // Physical width + viewBox give the mm-per-user-unit scale.
                    double const widthMm =
                        has( "width" ) ? detail::LengthToMm( av( "width" ).toString() ) : -1.0;
                    if( widthMm > 0.0 && has( "viewBox" ) ) {
                        std::vector< double > const vb =
                            detail::Numbers( av( "viewBox" ).toString() );
                        if( vb.size() >= 4 && vb[ 2 ] > 0.0 ) {
                            mmPerUnit = widthMm / vb[ 2 ];
                        }
                    }
                } else if( name == QLatin1String( "circle" ) ) {
                    bool oks = false;
                    double const cx = av( "cx" ).toDouble();
                    double const cy = av( "cy" ).toDouble();
                    double const r  = av( "r" ).toDouble( &oks );
                    if( oks && r > 0.0 ) {
                        double wx = 0.0, wy = 0.0;
                        detail::Apply( cur, cx, cy, wx, wy );
                        raw.push_back( { wx, wy, r * detail::ScaleOf( cur ) } );
                    }
                } else if( name == QLatin1String( "ellipse" ) ) {
                    double const cx = av( "cx" ).toDouble();
                    double const cy = av( "cy" ).toDouble();
                    bool oksx = false, oksy = false;
                    double const rx = av( "rx" ).toDouble( &oksx );
                    double const ry = av( "ry" ).toDouble( &oksy );
                    // Only near-circular ellipses count as holes.
                    if( oksx && oksy && rx > 0.0 && ry > 0.0
                        && std::abs( rx - ry ) <= 0.2 * std::max( rx, ry ) ) {
                        double wx = 0.0, wy = 0.0;
                        detail::Apply( cur, cx, cy, wx, wy );
                        raw.push_back( { wx, wy, 0.5 * ( rx + ry ) * detail::ScaleOf( cur ) } );
                    }
                }
            } else if( tok == QXmlStreamReader::EndElement ) {
                if( !stack.empty() ) {
                    stack.pop_back();
                }
            }
        }

        circles.reserve( raw.size() );
        for( Raw const& rc : raw ) {
            // To mm, flipping Y (SVG down -> node up).
            circles.push_back( { rc.x * mmPerUnit, -rc.y * mmPerUnit, 2.0 * rc.r * mmPerUnit } );
        }
        return circles;
    }
}  // namespace svg_reader
