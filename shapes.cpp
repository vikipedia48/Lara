#include "shapes.h"
#include "consts.h"
#include "qjsonarray.h"
#include <cfloat>

#define SIZE(wkbType) 16+8*(uint32_t)(((uint32_t)wkbType+1000)/2000)

Shape::GeometryType Shape::geometryTypeFromString(const QString &str)
{
    if (str == "Point") return GeometryType::Point;
    if (str == "MultiPoint") return GeometryType::MultiPoint;
    if (str == "LineString") return GeometryType::LineString;
    if (str == "MultiLineString") return GeometryType::MultiLineString;
    if (str == "Polygon") return GeometryType::Polygon;
    if (str == "MultiPolygon") return GeometryType::MultiPolygon;
    return GeometryType::Error;
}

Shape::Point::Point(const QJsonValue &coordinates) : Shape(GeometryType::Point)
{
    try {
        if (!coordinates.isArray()) throw std::invalid_argument("");
        auto array = coordinates.toArray();
        if (array.size() != 2) throw std::invalid_argument("");
        x = array[0].toDouble();
        y = array[1].toDouble();
    } catch(std::invalid_argument e) {
        type = GeometryType::Error;
    }
}

Util::Boundaries Shape::Point::getBoundaries()
{
    return {x,x,y,y};
}

void Shape::Point::drawShape(cimg_library::CImg<unsigned char> *img, const color &color, const GeoJsonConvertParams& params)
{
    drawShape(img, color, params.boundaries.value(), params.width, params.height);
}

void Shape::Point::drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const Util::Boundaries& boundaries, uint32_t width, uint32_t height) {
    auto minX = boundaries.minX;
    auto maxX = boundaries.maxX;
    auto minY = boundaries.minY;
    auto maxY = boundaries.maxY;
    if (x < minX || x > maxX || y < minY || y > maxY) return;
    img->draw_point(std::round(Util::Remap(x, minX, maxX, 0, width-1)),
                    std::round(Util::Remap(y, minY, maxY, 0, height-1)), color.data());
}

void Shape::Point::loadFromBlob(std::vector<unsigned char> &blob, size_t& startPos)
{
    bool littleEndian = blob[startPos] != 0;
    startPos+=1;
    uint32_t wkbType;
    Util::copyBytesToVar(blob, startPos, &wkbType, littleEndian);
    startPos+=4;
    auto coordinate = Coordinate(blob, startPos, littleEndian);
    startPos+=SIZE(wkbType);
    x = coordinate.x;
    y = coordinate.y;
}

Shape::MultiPoint::MultiPoint(const QJsonValue &coordinates) : Shape(GeometryType::MultiPoint)
{
    try {
        if (!coordinates.isArray()) throw std::invalid_argument("");
        auto array = coordinates.toArray();
        for (const auto v : array) {
            auto point = Point(v);
            if (point.type == GeometryType::Error) throw std::invalid_argument("");
            points.push_back(Point(point));
        }
    } catch(std::invalid_argument e) {
        type = GeometryType::Error;
    }
}


Util::Boundaries Shape::MultiPoint::getBoundaries()
{
    Util::Boundaries rv = {std::numeric_limits<double>::max(),std::numeric_limits<double>::lowest(),std::numeric_limits<double>::max(),std::numeric_limits<double>::lowest()};
    for (auto& v : points) {
        rv.minX = std::min(rv.minX, v.x);
        rv.minY = std::min(rv.minY, v.y);
        rv.maxX = std::max(rv.maxX, v.x);
        rv.maxY = std::max(rv.maxY, v.y);
    }
    return rv;
}

void Shape::MultiPoint::drawShape(cimg_library::CImg<unsigned char> *img, const color &color, const GeoJsonConvertParams& params)
{
    drawShape(img, color, params.boundaries.value(), params.width, params.height);
}

void Shape::MultiPoint::drawShape(cimg_library::CImg<unsigned char> *img, const color &color, const Util::Boundaries &boundaries, uint32_t width, uint32_t height)
{
    for (auto& v : points) {
        v.drawShape(img, color, boundaries, width, height);
    }
}

void Shape::MultiPoint::loadFromBlob(std::vector<unsigned char> &blob, size_t &startPos)
{
    bool littleEndian = blob[startPos] != 0;
    startPos+=1;
    uint32_t wkbType;
    Util::copyBytesToVar(blob, startPos, &wkbType, littleEndian);
    startPos+=4;
    uint32_t numPoints;
    Util::copyBytesToVar(blob, startPos, &numPoints, littleEndian);
    startPos+=4;
    for (auto i = 0; i < numPoints; ++i) {
        auto point = Point();
        point.loadFromBlob(blob, startPos);
        points.push_back(std::move(point));
    }
}

Shape::LineString::LineString(const QJsonValue &coordinates) : Shape(GeometryType::LineString)
{
    try {
        if (!coordinates.isArray()) throw std::invalid_argument("");
        auto array = coordinates.toArray();
        for (const auto v : array) {
            auto point = Point(v);
            if (point.type == GeometryType::Error) throw std::invalid_argument("");
            points.push_back(Point(point));
        }
    } catch(std::invalid_argument e) {
        type = GeometryType::Error;
    }
}

Shape::LineString::LineString(LinearRing &&ring) : Shape(GeometryType::LineString)
{
    for (auto& v : ring.points) {
        points.emplace_back(Point(v.x,v.y));
    }
}

Util::Boundaries Shape::LineString::getBoundaries()
{
    Util::Boundaries rv = {std::numeric_limits<double>::max(),std::numeric_limits<double>::lowest(),std::numeric_limits<double>::max(),std::numeric_limits<double>::lowest()};
    for (auto& v : points) {
        rv.minX = std::min(rv.minX, v.x);
        rv.minY = std::min(rv.minY, v.y);
        rv.maxX = std::max(rv.maxX, v.x);
        rv.maxY = std::max(rv.maxY, v.y);
    }
    return rv;
}

void Shape::LineString::drawShape(cimg_library::CImg<unsigned char> *img, const color &color, const GeoJsonConvertParams& params)
{
    drawShape(img, color, params.boundaries.value(), params.width, params.height);
}

void Shape::LineString::drawShape(cimg_library::CImg<unsigned char> *img, const color &color, const Util::Boundaries &boundaries, uint32_t width, uint32_t height)
{
    if (points.size() < 2) return;
    auto minX = boundaries.minX;
    auto maxX = boundaries.maxX;
    auto minY = boundaries.minY;
    auto maxY = boundaries.maxY;

    for (auto i = 0; i < points.size()-1; ++i) {
        if (points[i].x < minX || points[i].x > maxX || points[i].y < minY || points[i].y > maxY) continue;
        if (points[i+1].x < minX || points[i+1].x > maxX || points[i+1].y < minY || points[i+1].y > maxY) continue;
        auto x0 = std::round(Util::Remap(points[i].x, minX, maxX, 0, width-1));
        auto y0 = std::round(Util::Remap(points[i].y, minY, maxY, 0, height-1));
        auto x1 = std::round(Util::Remap(points[i+1].x, minX, maxX, 0, width-1));
        auto y1 = std::round(Util::Remap(points[i+1].y, minY, maxY, 0, height-1));
        img->draw_line(x0, y0, x1, y1, color.data());
    }
}

void Shape::LineString::loadFromBlob(std::vector<unsigned char> &blob, size_t &startPos)
{
    bool littleEndian = blob[startPos] != 0;
    startPos+=1;
    uint32_t wkbType;
    Util::copyBytesToVar(blob, startPos, &wkbType, littleEndian);
    startPos+=4;
    uint32_t numPoints;
    Util::copyBytesToVar(blob, startPos, &numPoints, littleEndian);
    startPos+=4;
    for (auto i = 0; i < numPoints; ++i) {
        auto coordinate = Coordinate(blob, startPos, littleEndian);
        startPos+=SIZE(wkbType);
        points.emplace_back(Point(coordinate.x, coordinate.y));
    }
}

Shape::MultiLineString::MultiLineString(const QJsonValue &coordinates) : Shape(GeometryType::MultiLineString)
{
    try {
        if (!coordinates.isArray()) throw std::invalid_argument("");
        auto array = coordinates.toArray();
        for (const auto v : array) {
            auto lineString = LineString(v);
            if (lineString.type == GeometryType::Error) throw std::invalid_argument("");
            lines.push_back(lineString);
        }
    } catch(std::invalid_argument e) {
        type = GeometryType::Error;
    }
}

Util::Boundaries Shape::MultiLineString::getBoundaries()
{
    Util::Boundaries rv = {std::numeric_limits<double>::max(),std::numeric_limits<double>::lowest(),std::numeric_limits<double>::max(),std::numeric_limits<double>::lowest()};
    for (auto& v : lines) {
        auto bounds = v.getBoundaries();
        rv.minX = std::min(rv.minX, bounds.minX);
        rv.minY = std::min(rv.minY, bounds.minY);
        rv.maxX = std::max(rv.maxX, bounds.maxX);
        rv.maxY = std::max(rv.maxY, bounds.maxY);
    }
    return rv;
}

void Shape::MultiLineString::drawShape(cimg_library::CImg<unsigned char> *img, const color &color, const GeoJsonConvertParams& params)
{
    for (auto& v : lines) {
        v.drawShape(img, color, params);
    }
}

void Shape::MultiLineString::drawShape(cimg_library::CImg<unsigned char> *img, const color &color, const Util::Boundaries &boundaries, uint32_t width, uint32_t height)
{
    for (auto& v : lines) {
        v.drawShape(img, color, boundaries, width, height);
    }
}

void Shape::MultiLineString::loadFromBlob(std::vector<unsigned char> &blob, size_t &startPos)
{
    bool littleEndian = blob[startPos] != 0;
    startPos+=1;
    uint32_t wkbType;
    Util::copyBytesToVar(blob, startPos, &wkbType, littleEndian);
    startPos+=4;
    uint32_t numPoints;
    Util::copyBytesToVar(blob, startPos, &numPoints, littleEndian);
    startPos+=4;
    for (auto i = 0; i < numPoints; ++i) {
        auto lineString = LineString();
        lineString.loadFromBlob(blob, startPos);
        lines.push_back(std::move(lineString));
    }
}

Shape::Polygon::Polygon(const QJsonValue &coordinates) : Shape(GeometryType::Polygon)
{
    try {
        if (!coordinates.isArray()) throw std::invalid_argument("");
        auto array = coordinates.toArray();
        auto index = 0;
        for (const auto v : array) {
            auto lineString = LineString(v);
            if (lineString.type == GeometryType::Error) throw std::invalid_argument("");
            if (lineString.points.size() < 4) throw std::invalid_argument(""); // geojson standard
            if (lineString.points[0] != lineString.points[lineString.points.size()-1]) throw std::invalid_argument(""); // geojson standard
            if (index == 0) exteriorRing = lineString;
            else interiorRings.push_back(lineString);
            ++index;
        }
    } catch(std::invalid_argument e) {
        type = GeometryType::Error;
    }
}

Util::Boundaries Shape::Polygon::getBoundaries()
{
    return exteriorRing.getBoundaries();
}

void Shape::Polygon::drawShape(cimg_library::CImg<unsigned char> *img, const color &color, const GeoJsonConvertParams& params)
{
    drawShape(img, color, params.boundaries.value(), params.width, params.height);
}

void Shape::Polygon::drawShape(cimg_library::CImg<unsigned char> *img, const color &color, const Util::Boundaries &boundaries, uint32_t width, uint32_t height)
{
    auto minX = boundaries.minX;
    auto maxX = boundaries.maxX;
    auto minY = boundaries.minY;
    auto maxY = boundaries.maxY;

    auto bounds = getBoundaries();
    if (bounds.minX < minX || bounds.maxX > maxX || bounds.minY < minY || bounds.maxY > maxY) return;

    cimg_library::CImg<int> points(exteriorRing.points.size(), 2);
    for (auto i = 0; i < exteriorRing.points.size(); ++i) {
        auto x = std::round(Util::Remap(exteriorRing.points[i].x, minX, maxX, 0, width-1));
        auto y = std::round(Util::Remap(exteriorRing.points[i].y, minY, maxY, 0, height-1));
        points(i,0) = x;
        points(i,1) = y;
    }

    // backing up existing data which is to be overwritten by polygon
    std::map<std::pair<unsigned int, unsigned int>, std::array<unsigned char,4>> nonNullValues;
    if (!interiorRings.empty()) {
        auto exteriorRingBounds = exteriorRing.getBoundaries();
        std::uint32_t minX = std::round(Util::Remap(exteriorRingBounds.minX, minX, maxX, 0, width-1));
        std::uint32_t minY = std::round(Util::Remap(exteriorRingBounds.minY, minY, maxY, 0, height-1));
        std::uint32_t maxX = std::round(Util::Remap(exteriorRingBounds.maxX, minX, maxX, 0, width-1));
        std::uint32_t maxY = std::round(Util::Remap(exteriorRingBounds.maxY, minY, maxY, 0, height-1));
        for (auto y = minY; y <= maxY; ++y) {
            for (auto x = minX; x <= maxX; ++x) {
                if ((*img)(x,y,0,0) == 0 && (*img)(x,y,0,1) == 0 && (*img)(x,y,0,2) == 0 && (*img)(x,y,0,3) == 0) continue;
                nonNullValues.insert({{x,y}, {(*img)(x,y,0,0),(*img)(x,y,0,1),(*img)(x,y,0,2),(*img)(x,y,0,3)}});
            }
        }
    }

    img->draw_polygon(points, color.data());

    unsigned char blank[4] = {0,0,0,0};

    for (auto& v : interiorRings) {
        cimg_library::CImg<int> points(v.points.size(), 2);
        for (auto i = 0; i < v.points.size(); ++i) {
            int x = std::round(Util::Remap(v.points[i].x, minX, maxX, 0, width-1));
            int y = std::round(Util::Remap(v.points[i].y, minY, maxY, 0, height-1));
            points(i,0) = x;
            points(i,1) = y;
        }
        img->draw_polygon(points, blank);

        // restore overwritten pixels
        if (nonNullValues.empty()) continue;

        auto interiorRingBounds = v.getBoundaries();
        std::uint32_t _minX = Util::Remap(interiorRingBounds.minX, minX, maxX, 0, width-1);
        std::uint32_t _minY = Util::Remap(interiorRingBounds.minY, minY, maxY, 0, height-1);
        std::uint32_t _maxX = Util::Remap(interiorRingBounds.maxX, minX, maxX, 0, width-1);
        std::uint32_t _maxY = Util::Remap(interiorRingBounds.maxY, minY, maxY, 0, height-1);
        for (auto it = nonNullValues.begin(); it != nonNullValues.end(); ++it) {
            auto point = it->first;
            if (point.first < _minX || point.first > _maxX || point.second < _minY || point.second > _maxY) continue;
            if ((*img)(point.first,point.second,0,0) == 0 && (*img)(point.first,point.second,0,1) == 0 && (*img)(point.first,point.second,0,2) == 0 && (*img)(point.first,point.second,0,3) == 0) {
                auto color = it->second;
                (*img)(point.first,point.second,0,0) = color[0];
                (*img)(point.first,point.second,0,1) = color[1];
                (*img)(point.first,point.second,0,2) = color[2];
                (*img)(point.first,point.second,0,3) = color[3];
            }
        }
    }
}

void Shape::Polygon::loadFromBlob(std::vector<unsigned char> &blob, size_t &startPos)
{
    bool littleEndian = blob[startPos] != 0;
    startPos+=1;
    uint32_t wkbType;
    Util::copyBytesToVar(blob, startPos, &wkbType, littleEndian);
    startPos+=4;
    uint32_t numPoints;
    Util::copyBytesToVar(blob, startPos, &numPoints, littleEndian);
    startPos+=4;
    auto ring = LinearRing(blob, startPos, wkbType, littleEndian);
    exteriorRing = LineString(std::move(ring));
    for (auto i = 1; i < numPoints; ++i) {
        auto interiorRing = LinearRing(blob, startPos, wkbType, littleEndian);
        interiorRings.emplace_back(LineString(std::move(interiorRing)));
    }
}

Shape::MultiPolygon::MultiPolygon(const QJsonValue &coordinates) : Shape(GeometryType::MultiPolygon)
{
    try {
        if (!coordinates.isArray()) throw std::invalid_argument("");
        auto array = coordinates.toArray();
        for (const auto v : array) {
            auto polygon = Polygon(v);
            if (polygon.type == GeometryType::Error) throw std::invalid_argument("");
            polygons.push_back(polygon);
        }
    } catch(std::invalid_argument e) {
        type = GeometryType::Error;
    }
}

Util::Boundaries Shape::MultiPolygon::getBoundaries() {
    Util::Boundaries rv = {std::numeric_limits<double>::max(),std::numeric_limits<double>::lowest(),std::numeric_limits<double>::max(),std::numeric_limits<double>::lowest()};
    for (auto& v : polygons) {
        auto bounds = v.getBoundaries();
        rv.minX = std::min(rv.minX, bounds.minX);
        rv.minY = std::min(rv.minY, bounds.minY);
        rv.maxX = std::max(rv.maxX, bounds.maxX);
        rv.maxY = std::max(rv.maxY, bounds.maxY);
    }
    return rv;
}

void Shape::MultiPolygon::drawShape(cimg_library::CImg<unsigned char> *img, const color &color, const GeoJsonConvertParams& params)
{
    for (auto& v : polygons) {
        v.drawShape(img, color, params);
    }
}

void Shape::MultiPolygon::drawShape(cimg_library::CImg<unsigned char> *img, const color &color, const Util::Boundaries &boundaries, uint32_t width, uint32_t height)
{
    for (auto& v : polygons) {
        v.drawShape(img, color, boundaries, width, height);
    }
}

void Shape::MultiPolygon::loadFromBlob(std::vector<unsigned char> &blob, size_t &startPos)
{
    bool littleEndian = blob[startPos] != 0;
    startPos+=1;
    uint32_t wkbType;
    Util::copyBytesToVar(blob, startPos, &wkbType, littleEndian);
    startPos+=4;
    uint32_t numPoints;
    Util::copyBytesToVar(blob, startPos, &numPoints, littleEndian);
    startPos+=4;
    for (auto i = 0; i < numPoints; ++i) {
        auto polygon = Polygon();
        polygon.loadFromBlob(blob, startPos);
        polygons.push_back(std::move(polygon));
    }
}

std::unique_ptr<Shape::Shape> Shape::fromWkbType(uint32_t wkbType)
{
    switch(wkbType) {
        case Gpkg::wkbPoint: case Gpkg::wkbPointZ: case Gpkg::wkbPointM: case Gpkg::wkbPointZM: return std::unique_ptr<Shape>(new Point());
        case Gpkg::wkbMultiPoint: case Gpkg::wkbMultiPointZ: case Gpkg::wkbMultiPointM: case Gpkg::wkbMultiPointZM: return std::unique_ptr<Shape>(new MultiPoint());
        case Gpkg::wkbPolygon: case Gpkg::wkbPolygonZ: case Gpkg::wkbPolygonM: case Gpkg::wkbPolygonZM: return std::unique_ptr<Shape>(new Polygon());
        case Gpkg::wkbMultiPolygon: case Gpkg::wkbMultiPolygonZ: case Gpkg::wkbMultiPolygonM: case Gpkg::wkbMultiPolygonZM: return std::unique_ptr<Shape>(new MultiPolygon());
        case Gpkg::wkbLineString: case Gpkg::wkbLineStringZ: case Gpkg::wkbLineStringM: case Gpkg::wkbLineStringZM: return std::unique_ptr<Shape>(new LineString());
        case Gpkg::wkbMultiLineString: case Gpkg::wkbMultiLineStringZ: case Gpkg::wkbMultiLineStringM: case Gpkg::wkbMultiLineStringZM: return std::unique_ptr<Shape>(new MultiLineString());
        default: throw std::invalid_argument("Unsupported geometry type found");
    }
}

Shape::Coordinate::Coordinate(std::vector<unsigned char> &blob, size_t startPos, bool littleEndian)
{
    Util::copyBytesToVar(blob, startPos, &x, littleEndian);
    Util::copyBytesToVar(blob, startPos+8, &y, littleEndian);
}

Shape::LinearRing::LinearRing(std::vector<unsigned char> &blob, size_t& startPos, uint32_t wkbType, bool littleEndian)
{
    Util::copyBytesToVar(blob, startPos, &numPoints, littleEndian);
    startPos+=4;
    for (auto i = 0; i < numPoints; ++i) {
        points.push_back(Coordinate(blob, startPos, littleEndian));
        startPos+=SIZE(wkbType);
    }
}
