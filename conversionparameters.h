#ifndef CONVERSIONPARAMETERS_H
#define CONVERSIONPARAMETERS_H


#include "commonfunctions.h"
#include <map>
#include <optional>

using color = std::array<unsigned char,4>;

struct TiffConvertParams {
    QString inputPath;
    unsigned int startX, startY, endX, endY;
    Util::OutputMode outputMode;
    Util::ScaleMode scaleMode;
    unsigned int scale;
    std::optional<std::pair<double,double>> minAndMax;
    std::optional<double> offset;
    std::optional<std::map<double,color>> colorValues;
    std::optional<bool> gradient;
    std::optional<std::string> luaFunction;
};

struct CsvConvertParams {
    QString inputPath;
    std::vector<std::pair<color, color>> colors;
    std::vector<std::pair<Util::CsvShapeType, unsigned int>> shapes;
    std::vector<std::pair<int,QString>> columnValues;
    std::array<unsigned int,2> coordinateIndexes;
    unsigned int width, height;
    std::optional<Util::Boundaries> boundaries;

};
struct NewCsvConvertParams {
    QString inputPath;
    std::array<unsigned int,2> coordinateIndexes;
    uint32_t width, height;
    std::optional<Util::Boundaries> boundaries;
    std::optional<std::string> luaScript;
};

struct GeoJsonConvertParams {
    QString inputPath;
    unsigned int width, height;
    std::vector<color> colors;
    std::vector<std::pair<QString, QString>> columnValues;
    std::optional<Util::Boundaries> boundaries;
};

struct NewGeoJsonConvertParams {
    QString inputPath;
    uint32_t width, height;
    std::optional<std::string> luaScript;
    std::optional<Util::Boundaries> boundaries;
};

struct LayerParams {
    std::vector<color> colors;
    std::vector<std::pair<std::string, std::string>> columnValues;
};

struct GeoPackageConvertParams {
    QString inputPath;
    uint32_t width, height;
    std::optional<Util::Boundaries> boundaries;
    std::vector<std::string> selectedLayers;
    std::optional<std::map<std::string, LayerParams>> layerParams;
};

struct NewGeoPackageConvertParams {
    QString inputPath;
    uint32_t width, height;
    std::optional<Util::Boundaries> boundaries;
    std::vector<std::string> selectedLayers;
    std::optional<std::string> luaScript;
};

#endif // CONVERSIONPARAMETERS_H
