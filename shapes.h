#ifndef SHAPES_H
#define SHAPES_H


#include <CImg.h>
#include "conversionparameters.h"

#include <QJsonValue>
#include <QString>
#include <vector>

using color = std::array<unsigned char,4>;

namespace Shape {

    struct Coordinate {
        double x, y;
        Coordinate(std::vector<unsigned char>& blob, size_t startPos, bool littleEndian);
    };
    struct LinearRing {
        uint32_t numPoints;
        std::vector<Coordinate> points;

        LinearRing(std::vector<unsigned char>& blob, size_t& startPos, uint32_t wkbType, bool littleEndian);
    };

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
        virtual void drawShape(cimg_library::CImg<uint8_t>* img,
                               const color& color, const GeoJsonConvertParams& params) = 0;
        virtual void drawShape(cimg_library::CImg<uint8_t>* img,
                               const color& color, const Util::Boundaries& boundaries,
                               uint32_t width, uint32_t height) = 0;
        virtual void loadFromBlob(std::vector<uint8_t>& blob, size_t& startPos) = 0;
        std::string geometryTypeToString();
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
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const Util::Boundaries& boundaries, uint32_t width, uint32_t height) override;
        void loadFromBlob(std::vector<unsigned char>& blob, size_t& startPos) override;
    };

    class MultiPoint : public Shape {
    public:
        std::vector<Point> points;
        MultiPoint() : Shape(GeometryType::MultiPoint) {}
        MultiPoint(const std::vector<Point>& points) : points(points), Shape(GeometryType::MultiPoint) {}
        MultiPoint(const QJsonValue& coordinates);
        Util::Boundaries getBoundaries() override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const Util::Boundaries& boundaries, uint32_t width, uint32_t height) override;
        void loadFromBlob(std::vector<unsigned char>& blob, size_t& startPos) override;
    };

    class LineString : public Shape {
    public:
        std::vector<Point> points;
        LineString() : Shape(GeometryType::LineString) {}
        LineString(const std::vector<Point>& points) : points(points), Shape(GeometryType::LineString) {}
        LineString(const QJsonValue& coordinates);
        LineString(std::vector<unsigned char>& blob, size_t startPos);
        LineString(LinearRing&& ring);
        Util::Boundaries getBoundaries() override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const Util::Boundaries& boundaries, uint32_t width, uint32_t height) override;
        void loadFromBlob(std::vector<unsigned char>& blob, size_t& startPos) override;
    };

    class MultiLineString : public Shape {
    public:
        std::vector<LineString> lines;
        MultiLineString() : Shape(GeometryType::MultiLineString) {}
        MultiLineString(const std::vector<LineString>& lines) : lines(lines), Shape(GeometryType::MultiLineString) {}
        MultiLineString(const QJsonValue& coordinates);
        MultiLineString(std::vector<unsigned char>& blob, size_t startPos);
        Util::Boundaries getBoundaries() override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const Util::Boundaries& boundaries, uint32_t width, uint32_t height) override;
        void loadFromBlob(std::vector<unsigned char>& blob, size_t& startPos) override;
    };

    class Polygon : public Shape {
    public:
        LineString exteriorRing;
        std::vector<LineString> interiorRings;
        Polygon() : Shape(GeometryType::Polygon) {}
        Polygon(const LineString& exteriorRing, const std::vector<LineString>& interiorRings) : exteriorRing(exteriorRing), interiorRings(interiorRings), Shape(GeometryType::Polygon) {}
        Polygon(const QJsonValue& coordinates);
        Polygon(std::vector<unsigned char>& blob, size_t startPos);
        Util::Boundaries getBoundaries() override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const Util::Boundaries& boundaries, uint32_t width, uint32_t height) override;
        void loadFromBlob(std::vector<unsigned char>& blob, size_t& startPos) override;
    };

    class MultiPolygon : public Shape {
    public:
        std::vector<Polygon> polygons;
        MultiPolygon() : Shape(GeometryType::MultiPolygon) {}
        MultiPolygon(const std::vector<Polygon>& polygons) : polygons(polygons), Shape(GeometryType::MultiPolygon) {}
        MultiPolygon(const QJsonValue& coordinates);
        MultiPolygon(std::vector<unsigned char>& blob, size_t startPos);
        Util::Boundaries getBoundaries() override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const GeoJsonConvertParams& params) override;
        void drawShape(cimg_library::CImg<unsigned char>* img, const color& color, const Util::Boundaries& boundaries, uint32_t width, uint32_t height) override;
        void loadFromBlob(std::vector<unsigned char>& blob, size_t& startPos) override;
    };

    GeometryType geometryTypeFromString(const QString& str);
    std::unique_ptr<Shape> fromWkbType(uint32_t wkbType);
    //std::unique_ptr<Shape> fromBlob(std::vector<unsigned char>& blob, size_t startPos);

}
#endif // SHAPES_H
