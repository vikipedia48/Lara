#ifndef SHAPES_H
#define SHAPES_H


#include <CImg.h>
#include "conversionparameters.h"

#include <QJsonValue>
#include <QString>
#include <vector>

using color = std::array<unsigned char,4>;

namespace Shape {

    enum class GeometryType {
        Error,
        Point,
        MultiPoint,
        LineString,
        MultiLineString,
        Polygon,
        MultiPolygon
    };

    class Shape {
    public:
        GeometryType type;
        int propertyId = -1;
        virtual Util::Boundaries getBoundaries() = 0;
        virtual void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) = 0;
    protected:
        Shape(GeometryType type) : type(type) {}
    };

    class Point : public Shape {
    public:
        double x,y;
        Point() : Shape(GeometryType::Point) {}
        Point(double x, double y) : x(x), y(y), Shape(GeometryType::Point) {}
        Point(const QJsonValue& coordinates);
        bool operator == (const Point& rhs) { return rhs.x == x && rhs.y == y; }
        bool operator != (const Point& rhs) { return rhs.x != x || rhs.y != y; }
        Util::Boundaries getBoundaries() override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) override;
    };

    class MultiPoint : public Shape {
    public:
        std::vector<Point> points;
        MultiPoint() : Shape(GeometryType::MultiPoint) {}
        MultiPoint(const std::vector<Point>& points) : points(points), Shape(GeometryType::MultiPoint) {}
        MultiPoint(const QJsonValue& coordinates);
        Util::Boundaries getBoundaries() override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) override;
    };

    class LineString : public Shape {
    public:
        std::vector<Point> points;
        LineString() : Shape(GeometryType::LineString) {}
        LineString(const std::vector<Point>& points) : points(points), Shape(GeometryType::LineString) {}
        LineString(const QJsonValue& coordinates);
        Util::Boundaries getBoundaries() override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) override;
    };

    class MultiLineString : public Shape {
    public:
        std::vector<LineString> lines;
        MultiLineString() : Shape(GeometryType::MultiLineString) {}
        MultiLineString(const std::vector<LineString>& lines) : lines(lines), Shape(GeometryType::MultiLineString) {}
        MultiLineString(const QJsonValue& coordinates);
        Util::Boundaries getBoundaries() override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) override;
    };

    class Polygon : public Shape {
    public:
        LineString exteriorRing;
        std::vector<LineString> interiorRings;
        Polygon() : Shape(GeometryType::Polygon) {}
        Polygon(const LineString& exteriorRing, const std::vector<LineString>& interiorRings) : exteriorRing(exteriorRing), interiorRings(interiorRings), Shape(GeometryType::Polygon) {}
        Polygon(const QJsonValue& coordinates);
        Util::Boundaries getBoundaries() override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) override;
    };

    class MultiPolygon : public Shape {
    public:
        std::vector<Polygon> polygons;
        MultiPolygon() : Shape(GeometryType::MultiPolygon) {}
        MultiPolygon(const std::vector<Polygon>& polygons) : polygons(polygons), Shape(GeometryType::MultiPolygon) {}
        MultiPolygon(const QJsonValue& coordinates);
        Util::Boundaries getBoundaries() override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) override;
    };

    GeometryType geometryTypeFromString(const QString& str);

}
#endif // SHAPES_H
