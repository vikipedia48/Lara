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

struct GeoJsonConvertParams {
    QString inputPath;
    unsigned int width, height;
    std::vector<color> colors;
    std::vector<std::pair<QString, QString>> columnValues;
    std::optional<Util::Boundaries> boundaries;
};

#endif // CONVERSIONPARAMETERS_H
