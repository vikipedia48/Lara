#ifndef IMAGECONVERTER_H
#define IMAGECONVERTER_H

#include "conversionparameters.h"
#include "shapes.h"

#include <CImg.h>
#include <QObject>
#include <set>

using color = std::array<unsigned char,4>;

class ImageConverter : public QObject
{
    Q_OBJECT
public:
signals:
    void sendProgress(uint32_t progress);
    void sendProgressReset(QString text);
    void sendProgressError();
public:
    ImageConverter() = default;

    bool GetMinAndMaxValues(const QString& path, double &min, double &max, int startX = 0, int endX = -1, int startY = 0, int endY = -1);
    std::set<double> GetDistinctValues(const QString& path);
    std::unique_ptr<double[]> GetRawImageValues(const QString& path, int startX, int endX, int startY, int endY);
    std::unique_ptr<unsigned short[]> CreateImageData_G16(double* rawValues, const TiffConvertParams& params, std::pair<unsigned int,unsigned int>& outWidthAndHeight);
    std::unique_ptr<unsigned char[]> CreateImageData_RGB(double* rawValues, const TiffConvertParams& params, std::pair<unsigned int,unsigned int>& outWidthAndHeight);

    std::unique_ptr<uint16_t[]> CreateG16_TrueValue(const QString& path, double offset, int startX, int startY, int endX, int endY);
    std::unique_ptr<uint16_t[]> CreateG16_MinToMax(const QString &path, const std::pair<double,double>& minAndMax, int startX, int startY, int endX, int endY);
    std::unique_ptr<uint8_t[]> CreateRGB_UserValues(const QString& path, const std::map<double,color>& colorValues, int startX, int startY, int endX, int endY);
    std::unique_ptr<uint8_t[]> CreateRGB_UserRanges(const QString& path, const std::map<double,color>& colorValues, bool useGradient, int startX, int startY, int endX, int endY);
    std::unique_ptr<uint8_t[]> CreateRGB_Formula(const QString& path, int startX, int startY, int endX, int endY);
    std::unique_ptr<uint8_t[]> CreateRGB_Points(const CsvConvertParams& params);
    cimg_library::CImg<uint8_t> CreateRGB_VectorShapes(GeoJsonConvertParams params, bool flipY = true); // pass by value
    cimg_library::CImg<uint8_t> CreateRGB_GeoPackage(GeoPackageConvertParams params, bool flipY = true); // pass by value

    std::vector<std::unique_ptr<Shape::Shape>> getAllShapesFromJson(const QString& path, std::optional<Util::Boundaries>& boundaries, std::vector<QJsonObject>& outputProperties);
    std::vector<std::unique_ptr<Shape::Shape>> getAllShapesFromLayer(const QString& path, std::string layerName, GeoPackageConvertParams& params, std::vector<color>& outputColors, boolean calculateBoundaries);

    static uint32_t readWKBGeometry(std::vector<unsigned char>&& bytes, std::vector<std::unique_ptr<Shape::Shape>>& outShapes, int envelopeSize, int row);
    static std::vector<std::vector<std::string>> getRows(const std::string& path);
    static Util::Boundaries getBoundaries(const std::vector<std::vector<std::string>>& rows, const std::array<unsigned int,2>& indexes, bool& ok);
    static std::unique_ptr<Shape::Shape> getShapeFromJson(const QJsonValue& json);
    static color getColorForVectorShape(Shape::Shape* shape, const GeoPackageConvertParams& params, const std::vector<std::string>& properties, const std::string& layerName, const std::vector<std::string>& allColumns);
    static color getColorForVectorShape(Shape::Shape* shape, const GeoJsonConvertParams& params, const std::vector<QJsonObject>& properties);
    static int getStyleForCsvShape(const std::vector<std::string>& csvRow, const CsvConvertParams& params);
    static std::uint16_t transformCellToG16TrueValue (double cell, double offset);
    static std::uint16_t transformCellToG16MinToMax (double cell, const std::pair<double,double>& minAndMax);
    static color transformCellToRGBUserValues(double cell, const std::map<double,color>& colorValues);
    static color transformCellToRGBUserRanges(double cell, const std::map<double,color>& colorValues, bool useGradient);
    static color transformCellToRGBFormula(double cell);
};

#endif // IMAGECONVERTER_H
