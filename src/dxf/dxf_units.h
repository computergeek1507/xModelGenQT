#pragma once

namespace dxf_units
{
    // DXF $INSUNITS codes for the units this app cares about.
    enum Code
    {
        Unitless    = 0,
        Inches      = 1,
        Feet        = 2,
        Millimeters = 4,
        Centimeters = 5,
        Meters      = 6,
        Microns     = 13,
        Decimeters  = 14,
    };

    // Millimeters represented by a single unit of the given DXF $INSUNITS code.
    // Returns 0.0 when the units are unitless/unknown (i.e. no real-world scale).
    inline double MillimetersPerUnit( int insUnits )
    {
        switch( insUnits ) {
            case Inches:      return 25.4;
            case Feet:        return 304.8;
            case Millimeters: return 1.0;
            case Centimeters: return 10.0;
            case Meters:      return 1000.0;
            case Microns:     return 0.001;
            case Decimeters:  return 100.0;
            default:          return 0.0;  // unitless / unsupported
        }
    }

    // Short human-readable name for a $INSUNITS code (for status messages).
    inline char const* UnitName( int insUnits )
    {
        switch( insUnits ) {
            case Inches:      return "inches";
            case Feet:        return "feet";
            case Millimeters: return "mm";
            case Centimeters: return "cm";
            case Meters:      return "m";
            case Microns:     return "microns";
            case Decimeters:  return "dm";
            default:          return "units";
        }
    }

    // Convert a real-world length (given in the units of realWorldUnitCode) into the
    // drawing units of a file whose units are drawingUnitCode. Returns a negative
    // value when the drawing units are unknown, so the caller can detect that case.
    inline double ToDrawingUnits( double realWorldValue, int realWorldUnitCode, int drawingUnitCode )
    {
        double const mmPerDrawingUnit = MillimetersPerUnit( drawingUnitCode );
        if( mmPerDrawingUnit <= 0.0 ) {
            return -1.0;  // drawing units unknown — cannot convert
        }

        double const realWorldMm = realWorldValue * MillimetersPerUnit( realWorldUnitCode );
        return realWorldMm / mmPerDrawingUnit;
    }
}
