#ifndef CONSTS_H
#define CONSTS_H

#include <cstdint>

namespace Gpkg {
    constexpr uint32_t wkbPoint = 1;
    constexpr uint32_t wkbLineString = 2;
    constexpr uint32_t wkbPolygon = 3;
    constexpr uint32_t wkbTriangle = 1;
    constexpr uint32_t wkbMultiPoint = 4;
    constexpr uint32_t wkbMultiLineString = 5;
    constexpr uint32_t wkbMultiPolygon = 6;
    constexpr uint32_t wkbGeometryCollection = 7;
    constexpr uint32_t wkbPolyhedralSurface = 15;
    constexpr uint32_t wkbTIN = 1;
    constexpr uint32_t wkbPointZ = 1001;
    constexpr uint32_t wkbLineStringZ = 1002;
    constexpr uint32_t wkbPolygonZ = 1003;
    constexpr uint32_t wkbTrianglez = 101;
    constexpr uint32_t wkbMultiPointZ = 1004;
    constexpr uint32_t wkbMultiLineStringZ = 1005;
    constexpr uint32_t wkbMultiPolygonZ = 1006;
    constexpr uint32_t wkbGeometryCollectionZ = 1007;
    constexpr uint32_t wkbPolyhedralSurfaceZ = 1015;
    constexpr uint32_t wkbTINZ = 101;
    constexpr uint32_t wkbPointM = 2001;
    constexpr uint32_t wkbLineStringM = 2002;
    constexpr uint32_t wkbPolygonM = 2003;
    constexpr uint32_t wkbTriangleM = 201;
    constexpr uint32_t wkbMultiPointM = 2004;
    constexpr uint32_t wkbMultiLineStringM = 2005;
    constexpr uint32_t wkbMultiPolygonM = 2006;
    constexpr uint32_t wkbGeometryCollectionM = 2007;
    constexpr uint32_t wkbPolyhedralSurfaceM = 2015;
    constexpr uint32_t wkbTINM = 201;
    constexpr uint32_t wkbPointZM = 3001;
    constexpr uint32_t wkbLineStringZM = 3002;
    constexpr uint32_t wkbPolygonZM = 3003;
    constexpr uint32_t wkbTriangleZM = 301;
    constexpr uint32_t wkbMultiPointZM = 3004;
    constexpr uint32_t wkbMultiLineStringZM = 3005;
    constexpr uint32_t wkbMultiPolygonZM = 3006;
    constexpr uint32_t wkbGeometryCollectionZM = 3007;
    constexpr uint32_t wkbPolyhedralSurfaceZM = 3015;
    constexpr uint32_t wkbTinZM = 3016;

    constexpr uint8_t magic_1 = 0x47;
    constexpr uint8_t magic_2 = 0x50;
    constexpr auto envelope32 = 0b00000010;
    constexpr auto envelope48_1 = 0b00000100;
    constexpr auto envelope48_2 = 0b00000110;
    constexpr auto envelope64 = 0b00001000;

}

#endif // CONSTS_H
