#include "imageconverter.h"

#include "consts.h"
#include "pngfunctions.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "qtfunctions.h"
#include "tifffunctions.h"
#include "commonfunctions.h"
#include <limits.h>

#include <QFile>
#include <QJsonObject>
#include <QThreadPool>
#include <bitset>
#include <rapidcsv.h>

#include <sqlite3/sqlite_modern_cpp.h>


std::vector<std::vector<std::string> > ImageConverter::getRows(const std::string& path)
{
    std::vector<std::vector<std::string>> rv;
    rapidcsv::Document doc(path);
    auto columnCount = doc.GetColumnCount();
    auto rowCount = doc.GetRowCount();
    if (columnCount == 0 || rowCount == 0) {
        Gui::ThrowError("Invalid CSV file");
        return {};
    }
    if (columnCount == 1) {
        Gui::ThrowError("CSV file is valid, but has only one column, whereas it requires at least two (for x and y values)");
        return {};
    }
    for (auto i = 0; i < rowCount; ++i) {
        rv.emplace_back(doc.GetRow<std::string>(i));
    }
    return rv;
}


Util::Boundaries ImageConverter::getBoundaries(const std::vector<std::vector<std::string> > &rows, const std::array<unsigned int,2>& indexes, bool& ok)
{
    std::array<double,4> rv;
    std::optional<double> minX, maxX, minY, maxY;
    try {
        for (auto& row : rows) {
            double x = std::stod(row[indexes[0]]);
            double y = std::stod(row[indexes[1]]);
            if (x < minX.value_or(DBL_MAX)) minX = x;
            if (y < minY.value_or(DBL_MAX)) minY = y;
            if (x > maxX.value_or(-DBL_MAX)) maxX = x;
            if (y > maxY.value_or(-DBL_MAX)) maxY = y;
        }
        if (!minX.has_value() || !maxX.has_value() || !minY.has_value() || !maxY.has_value()) throw std::invalid_argument("");
        ok = true;
        return {minX.value(), maxX.value(), minY.value(), maxY.value()};
    } catch(std::invalid_argument e) {
        ok = false;
        return {0,0,0,0};
    }
}

std::unique_ptr<Shape::Shape> ImageConverter::getShapeFromJson(const QJsonValue& json)
{
    try {
        if (!json.isObject()) throw std::invalid_argument("");
        auto jsonObj = json.toObject();
        auto coordinates = jsonObj["coordinates"];
        if (coordinates == QJsonValue::Undefined || !coordinates.isArray()) throw std::invalid_argument("");
        auto type = jsonObj["type"];
        if (type == QJsonValue::Undefined) throw std::invalid_argument("");
        auto typeString = type.toString();
        auto geometryType = Shape::geometryTypeFromString(typeString);
        if (geometryType == Shape::GeometryType::Point) {
            auto point = new Shape::Point(coordinates);
            if (point->type == Shape::GeometryType::Error) {
                delete point;
                throw std::invalid_argument("");
            }
            return std::unique_ptr<Shape::Shape>(point);
        }
        if (geometryType == Shape::GeometryType::MultiPoint) {
            auto multiPoint = new Shape::MultiPoint(coordinates);
            if (multiPoint->type == Shape::GeometryType::Error) {
                delete multiPoint;
                throw std::invalid_argument("");
            }
            return std::unique_ptr<Shape::Shape>(multiPoint);
        }
        if (geometryType == Shape::GeometryType::LineString) {
            auto lineString = new Shape::LineString(coordinates);
            if (lineString->type == Shape::GeometryType::Error) {
                delete lineString;
                throw std::invalid_argument("");
            }
            return std::unique_ptr<Shape::Shape>(lineString);
        }
        if (geometryType == Shape::GeometryType::MultiLineString) {
            auto multiLineString = new Shape::MultiLineString(coordinates);
            if (multiLineString->type == Shape::GeometryType::Error) {
                delete multiLineString;
                throw std::invalid_argument("");
            }
            return std::unique_ptr<Shape::Shape>(multiLineString);
        }
        if (geometryType == Shape::GeometryType::Polygon) {
            auto polygon = new Shape::Polygon(coordinates);
            if (polygon->type == Shape::GeometryType::Error) {
                delete polygon;
                throw std::invalid_argument("");
            }
            return std::unique_ptr<Shape::Shape>(polygon);
        }
        if (geometryType == Shape::GeometryType::MultiPolygon) {
            auto multiPolygon = new Shape::MultiPolygon(coordinates);
            if (multiPolygon->type == Shape::GeometryType::Error) {
                delete multiPolygon;
                throw std::invalid_argument("");
            }
            return std::unique_ptr<Shape::Shape>(multiPolygon);
        }
        throw std::invalid_argument("");
    } catch (std::invalid_argument e) {
        return nullptr;
    }
    return nullptr;
}

color ImageConverter::getColorForVectorShape(Shape::Shape *shape, const GeoPackageConvertParams &params, const std::vector<std::string> &properties, const std::string& layerName, const std::vector<std::string>& allColumns)
{
    if (params.layerParams.value().count(layerName) == 0) {
        auto layerParams = params.layerParams.value().at("!ALL LAYERS!");
        if (layerParams.colors.size() == 1) return layerParams.colors[0];
        for (auto i = 1; i < layerParams.columnValues.size(); ++i) {
            if (layerParams.columnValues[i].first == "!LAYER NAME!" && layerName == layerParams.columnValues[i].second) {
                return layerParams.colors[i];
            }
            else if (layerParams.columnValues[i].first == "!GEOMETRY TYPE!" && shape->type == Shape::geometryTypeFromString(QString::fromStdString(layerParams.columnValues[i].second))) {
                return layerParams.colors[i];
            }
        }
        return layerParams.colors[0];
    }
    else {
        auto layerParams = params.layerParams.value().at(layerName);
        if (layerParams.colors.size() == 1) return layerParams.colors[0];
        for (auto i = 1; i < layerParams.columnValues.size(); ++i) {
            auto it = std::find(allColumns.begin(), allColumns.end(), layerParams.columnValues[i].first);
            if (properties[it-allColumns.begin()] == layerParams.columnValues[i].second) {
                return layerParams.colors[i];
            }
        }
        return layerParams.colors[0];
    }
}

color ImageConverter::getColorForVectorShape(Shape::Shape* shape, const GeoJsonConvertParams& params, const std::vector<QJsonObject> &properties)
{
    auto propertyId = shape->propertyId;
    auto type = shape->type;
    if (propertyId == -1 || type == Shape::GeometryType::Error || params.colors.size() == 0) return {0,0,0,0};
    if (params.colors.size() == 1) return params.colors[0];

    for (auto i = 1; i < params.columnValues.size(); ++i) {

        if (params.columnValues[i].first == "!GEOMETRY TYPE!") {
            auto gType = Shape::geometryTypeFromString(params.columnValues[i].second);
            if (gType == Shape::GeometryType::Error) {
                return params.colors[0];
            }
            if (type == gType) return params.colors[i];
        }

        auto propertyValue = properties[propertyId].value(params.columnValues[i].first);

        if (propertyValue.isDouble()) {
            bool ok;
            auto requiredValue = params.columnValues[i].second.toDouble(&ok);
            if (!ok) return params.colors[0];
            if (requiredValue == propertyValue.toDouble()) return params.colors[i];
        }

        else if (propertyValue.isBool()) {
            if (params.columnValues[i].second == "true") {
                if (propertyValue.toBool()) return params.colors[i];
            }
            else if (params.columnValues[i].second == "false") {
                if (!propertyValue.toBool()) return params.colors[i];
            }
            else return params.colors[0];
        }

        else if (propertyValue.isString()) {
            if (propertyValue.toString() == params.columnValues[i].second) return params.colors[i];
        }

        //else continue;
    }
    return params.colors[0];
}

int ImageConverter::getStyleForCsvShape(const std::vector<std::string> &csvRow, const CsvConvertParams &params)
{
    if (params.colors.size() == 1) return 0;

    for (auto i = 1; i < params.columnValues.size(); ++i) {
        auto propertyValue = csvRow[params.columnValues[i].first];
        auto requiredValue = params.columnValues[i].second;
        if (requiredValue.toStdString() == propertyValue) {
            return i;
        }
    }

    return 0;
}

uint16_t ImageConverter::transformCellToG16TrueValue(double cell, double offset)
{
    uint16_t value;
    if (cell+offset < 0) value = 0;
    else if (cell+offset > std::numeric_limits<uint16_t>::max()) value = std::numeric_limits<uint16_t>::max();
    else value = (uint16_t)(cell+offset);
    return value;
}

uint16_t ImageConverter::transformCellToG16MinToMax(double cell, const std::pair<double, double>& minAndMax)
{
    auto remapped = Util::Remap(cell, minAndMax.first, minAndMax.second, 0, std::numeric_limits<uint16_t>::max());
    uint16_t value = round(remapped);
    return value;
}

color ImageConverter::transformCellToRGBUserValues(double cell, const std::map<double, color> &colorValues)
{
    return colorValues.at(cell);
}

color ImageConverter::transformCellToRGBUserRanges(double cell, const std::map<double, color> &colorValues, bool useGradient)
{
    color ar = {0,0,0,0};
    for (auto it = colorValues.begin(); it != colorValues.end();) {

        if (cell > it->first) {
            ++it;
            continue;
        }
        if (!useGradient || cell == it->first) ar = it->second;
        else {
            auto previousIt = it;
            --previousIt;
            double from1 = previousIt->first, to1 = it->first;
            color from2 = previousIt->second, to2 = it->second;
            ar[0] = Util::Remap(cell, from1, to1, from2[0], to2[0]);
            ar[1] = Util::Remap(cell, from1, to1, from2[1], to2[1]);
            ar[2] = Util::Remap(cell, from1, to1, from2[2], to2[2]);
            ar[3] = Util::Remap(cell, from1, to1, from2[3], to2[3]);
        }
        break;
    }
    return ar;
}

color ImageConverter::transformCellToRGBFormula(double cell)
{
    std::int32_t value = cell;
    color ar = {0,0,0,255};
    if (value < 0) {
        value*=-1;
        ar[3] = 128;
    }
    if (value >= 256*256*256) ar = {255,255,255, ar[3]};
    else {
        ar[0] = value/(256*256);
        value-=(256*256);
        ar[1] = value/256;
        ar[2] = value%256;
    }
    return ar;
}

std::unique_ptr<uint16_t[]> ImageConverter::CreateImageData_G16(double* rawValues, const TiffConvertParams &params, std::pair<unsigned int,unsigned int>& outWidthAndHeight)
{
    if (rawValues == nullptr) return {};

    auto rawWidth = (params.endX-params.startX+1);
    auto rawHeight = (params.endY-params.startY+1);
    unsigned int width, height;
    switch (params.scaleMode) {
        case Util::ScaleMode::No:
            width = rawWidth;
            height = rawHeight;
            break;
        case Util::ScaleMode::Decrease:
            width = std::ceil((float)rawWidth/params.scale);
            height = std::ceil((float)rawHeight/params.scale);
            break;
        case Util::ScaleMode::Increase:
            width = rawWidth*params.scale;
            height = rawHeight*params.scale;
            break;
    }

    auto buf = std::unique_ptr<unsigned short[]>(new unsigned short[width*height]);

    auto threadCount = std::thread::hardware_concurrency();
    std::vector<std::thread> threads; threads.reserve(threadCount);
    for (auto t = 0; t < threadCount; ++t) {
        threads.emplace_back([t, threadCount, width, height, rawWidth, rawHeight, &rawValues, &buf, &params, this]() {
            double cell;
            std::uint16_t value;
            size_t threadBegin, threadEnd;
            switch(params.scaleMode) {
                case Util::ScaleMode::No:
                    threadBegin = (float)t/threadCount*width*height;
                    threadEnd = (float)(t+1)/threadCount*width*height;
                    for (size_t i = threadBegin; i < threadEnd; ++i) {
                        cell = rawValues[i];
                        switch(params.outputMode) {
                            case Util::OutputMode::Grayscale16_MinToMax:
                                buf[i] = transformCellToG16MinToMax(cell, params.minAndMax.value());
                                break;
                            case Util::OutputMode::Grayscale16_TrueValue:
                                buf[i] = transformCellToG16TrueValue(cell, params.offset.value());
                                break;
                            default:
                                throw std::invalid_argument("unreachable code");
                        }
                        if (t == 0) {
                            float br = (float)i+1;
                            float nz = (float)width*height/threadCount;
                            emit sendProgress(br/nz*100);
                        }
                    }

                    break;
                case Util::ScaleMode::Decrease:
                    threadBegin = (float)t/threadCount*height;
                    threadEnd = (float)(t+1)/threadCount*height;
                    for (auto j = threadBegin; j < threadEnd; ++j) {
                        for (auto i = 0; i < width; ++i) {
                            cell = 0;
                            auto cellCount = 0;
                            for (auto l = 0; l < params.scale; ++l) {
                                for (auto k = 0; k < params.scale; ++k) {
                                    auto rawValueX = i*params.scale+k;
                                    auto rawValueY = j*params.scale+l;
                                    if (rawValueX >= rawWidth || rawValueY >= rawHeight) continue;
                                    cell+=rawValues[rawValueY*rawWidth+rawValueX];
                                    ++cellCount;
                                }
                            }
                            cell /= cellCount;
                            switch(params.outputMode) {
                                case Util::OutputMode::Grayscale16_MinToMax:
                                    buf[j*width+i] = transformCellToG16MinToMax(cell, params.minAndMax.value());
                                    break;
                                case Util::OutputMode::Grayscale16_TrueValue:
                                    buf[j*width+i] = transformCellToG16TrueValue(cell, params.offset.value());
                                    break;
                                default:
                                    throw std::invalid_argument("unreachable code");
                            }
                        }
                        if (t == 0) {
                            float br = (float)j+1;
                            float nz = (float)height/threadCount;
                            emit sendProgress(br/nz*100);
                        }
                    }
                    break;
                case Util::ScaleMode::Increase:
                    threadBegin = (float)t/threadCount*rawHeight;
                    threadEnd = (float)(t+1)/threadCount*rawHeight;
                    for (auto j = threadBegin; j < threadEnd; ++j) {
                        for (auto i = 0; i < rawWidth; ++i) {
                            cell = rawValues[j*rawWidth+i];
                            switch(params.outputMode) {
                                case Util::OutputMode::Grayscale16_MinToMax:
                                    value = transformCellToG16MinToMax(cell, params.minAndMax.value());
                                    break;
                                case Util::OutputMode::Grayscale16_TrueValue:
                                    value = transformCellToG16TrueValue(cell, params.offset.value());
                                    break;
                                default:
                                    throw std::invalid_argument("unreachable code");
                            }
                            for (auto l = 0; l < params.scale; ++l) {
                                for (auto k = 0; k < params.scale; ++k) {
                                    buf[(j*params.scale+l)*width+i*params.scale+k] = value;
                                }
                            }
                        }
                        if (t == 0) {
                            float br = (float)j+1;
                            float nz = (float)rawHeight/threadCount;
                            emit sendProgress(br/nz*100);
                        }
                    }
                    break;
            }
        });
    }

    for (auto& thread : threads) thread.join();

    outWidthAndHeight = {width, height};
    return buf;
}


std::unique_ptr<uint8_t[]> ImageConverter::CreateImageData_RGB(double* rawValues, const TiffConvertParams &params, std::pair<unsigned int,unsigned int>& outWidthAndHeight)
{
    if (rawValues == nullptr) return {};

    auto rawWidth = (params.endX-params.startX+1);
    auto rawHeight = (params.endY-params.startY+1);
    unsigned int width, height;
    switch (params.scaleMode) {
        case Util::ScaleMode::No:
            width = rawWidth;
            height = rawHeight;
            break;
        case Util::ScaleMode::Decrease:
            width = std::ceil((float)rawWidth/params.scale);
            height = std::ceil((float)rawHeight/params.scale);
            break;
        case Util::ScaleMode::Increase:
            width = rawWidth*params.scale;
            height = rawHeight*params.scale;
            break;
    }

    auto numberOfPixels = width*height;
    constexpr auto numberOfChannels = 4;
    auto buf = std::unique_ptr<unsigned char[]>(new unsigned char[numberOfChannels*numberOfPixels]);

    auto threadCount = std::thread::hardware_concurrency();
    std::vector<std::thread> threads; threads.reserve(threadCount);
    for (auto t = 0; t < threadCount; ++t) {
        threads.emplace_back([t, threadCount, width, height, rawWidth, rawHeight, numberOfPixels, &params, &buf, &rawValues, this]() {
            double cell;
            color value;
            size_t threadBegin;
            size_t threadEnd;
            switch(params.scaleMode) {
                case Util::ScaleMode::No:
                    threadBegin = (float)t/threadCount*numberOfPixels;
                    threadEnd = (float)(t+1)/threadCount*numberOfPixels;
                    for (auto i = threadBegin; i < threadEnd; ++i) {
                        cell = rawValues[i];
                        switch(params.outputMode) {
                            case Util::OutputMode::RGB_UserValues:
                                value = transformCellToRGBUserValues(cell, params.colorValues.value());
                                break;
                            case Util::OutputMode::RGB_UserRanges:
                                value = transformCellToRGBUserRanges(cell, params.colorValues.value(), params.gradient.value());
                                break;
                            case Util::OutputMode::RGB_Formula:
                                value = transformCellToRGBFormula(cell);
                                break;
                            default:
                                throw std::invalid_argument("unreachable code");
                        }
                        buf[i] = value[0];
                        buf[i+1*numberOfPixels] = value[1];
                        buf[i+2*numberOfPixels] = value[2];
                        buf[i+3*numberOfPixels] = value[3];
                        if (t == 0) {
                            float br = (float)i+1;
                            float nz = (float)numberOfPixels/threadCount;
                            emit sendProgress(br/nz*100);
                        }
                    }
                    break;
                case Util::ScaleMode::Decrease:
                    threadBegin = (float)t/threadCount*height;
                    threadEnd = (float)(t+1)/threadCount*height;
                    for (auto j = threadBegin; j < threadEnd; ++j) {
                        for (auto i = 0; i < width; ++i) {
                            cell = 0;
                            auto cellCount = 0;
                            for (auto l = 0; l < params.scale; ++l) {
                                for (auto k = 0; k < params.scale; ++k) {
                                    auto rawValueX = i*params.scale+k;
                                    auto rawValueY = j*params.scale+l;
                                    if (rawValueX >= rawWidth || rawValueY >= rawHeight) continue;
                                    cell+=rawValues[rawValueY*rawWidth+rawValueX];
                                    ++cellCount;
                                }
                            }
                            cell /= cellCount;
                            switch(params.outputMode) {
                                case Util::OutputMode::RGB_UserValues:
                                    value = transformCellToRGBUserValues(cell, params.colorValues.value());
                                    break;
                                case Util::OutputMode::RGB_UserRanges:
                                    value = transformCellToRGBUserRanges(cell, params.colorValues.value(), params.gradient.value());
                                    break;
                                case Util::OutputMode::RGB_Formula:
                                    value = transformCellToRGBFormula(cell);
                                    break;
                                default:
                                    throw std::invalid_argument("unreachable code");
                            }
                            buf[j*width+i] = value[0];
                            buf[j*width+i+1*numberOfPixels] = value[1];
                            buf[j*width+i+2*numberOfPixels] = value[2];
                            buf[j*width+i+3*numberOfPixels] = value[3];
                            if (t == 0) {
                                float br = (float)j+1;
                                float nz = (float)height/threadCount;
                                emit sendProgress(br/nz*100);
                            }
                        }
                    }
                    break;
                case Util::ScaleMode::Increase:
                    threadBegin = (float)t/threadCount*rawHeight;
                    threadEnd = (float)(t+1)/threadCount*rawHeight;
                    for (auto j = threadBegin; j < threadEnd; ++j) {
                        for (auto i = 0; i < rawWidth; ++i) {
                            cell = rawValues[j*rawWidth+i];
                            switch(params.outputMode) {
                                case Util::OutputMode::RGB_UserValues:
                                    value = transformCellToRGBUserValues(cell, params.colorValues.value());
                                    break;
                                case Util::OutputMode::RGB_UserRanges:
                                    value = transformCellToRGBUserRanges(cell, params.colorValues.value(), params.gradient.value());
                                    break;
                                case Util::OutputMode::RGB_Formula:
                                    value = transformCellToRGBFormula(cell);
                                    break;
                                default:
                                    throw std::invalid_argument("unreachable code");
                            }
                            for (auto l = 0; l < params.scale; ++l) {
                                for (auto k = 0; k < params.scale; ++k) {
                                    buf[(j*params.scale+l)*width+i*params.scale+k] = value[0];
                                    buf[(j*params.scale+l)*width+i*params.scale+k+1*numberOfPixels] = value[1];
                                    buf[(j*params.scale+l)*width+i*params.scale+k+2*numberOfPixels] = value[2];
                                    buf[(j*params.scale+l)*width+i*params.scale+k+3*numberOfPixels] = value[3];
                                }
                            }
                        }
                        if (t == 0) {
                            float br = (float)j+1;
                            float nz = (float)rawHeight/threadCount;
                            emit sendProgress(br/nz*100);
                        }
                    }
                    break;
        }
        });
    }
    for (auto& thread : threads) thread.join();

    outWidthAndHeight = {width, height};
    return buf;
}

std::unique_ptr<uint16_t[]> ImageConverter::CreateG16_MinToMax(const QString &path, const std::pair<double, double>& minAndMax, int startX, int startY, int endX, int endY)
{
    auto width = (endX-startX+1);
    auto height = (endY-startY+1);
    auto buf = std::unique_ptr<uint16_t[]>(new uint16_t[width*height]);
    if (!Tiff::LoadTiff(path,
        [&buf,minAndMax, startX, endX, startY, endY, width](std::vector<std::vector<double>>&& pixels, unsigned int _startX, unsigned int _startY) {
            for (auto _y = 0; _y < pixels.size(); ++_y) {
                if (_startY+_y < startY || _startY+_y > endY) continue;
                for (auto _x = 0; _x < pixels[0].size(); ++_x) {
                    if (_startX+_x < startX || _startX+_x > endX) continue;
                    auto x = _startX+_x-startX;
                    auto y = _startY+_y-startY;
                    buf[y*width+x] = transformCellToG16MinToMax(pixels[x][y], minAndMax);
                }
            }
        },
        [&buf,minAndMax, startX, endX, startY, width](std::vector<double>&& pixels, uint32_t row) {
            for (auto i = 0; i < pixels.size(); ++i) {
                buf[(row-startY)*width+i] = transformCellToG16MinToMax(pixels[i], minAndMax);
            }
        },
        [this](uint32_t percent) {
            emit sendProgress(percent);
        },
        startY, endY, startX, endX)) {
            return {};
            emit sendProgressError();
        }
    return buf;
}

std::unique_ptr<uint16_t[]> ImageConverter::CreateG16_TrueValue(const QString &path, double offset, int startX, int startY, int endX, int endY)
{
    auto width = (endX-startX+1);
    auto height = (endY-startY+1);
    auto buf = std::unique_ptr<uint16_t[]>(new uint16_t[width*height]);
    if (!Tiff::LoadTiff(path,
        [&buf,offset, startX, endX, startY, endY, width](std::vector<std::vector<double>>&& pixels, unsigned int _startX, unsigned int _startY) {
            for (auto _y = 0; _y < pixels.size(); ++_y) {
                if (_startY+_y < startY || _startY+_y > endY) continue;
                for (auto _x = 0; _x < pixels[0].size(); ++_x) {
                    if (_startX+_x < startX || _startX+_x > endX) continue;
                    auto x = _startX+_x-startX;
                    auto y = _startY+_y-startY;
                    buf[y*width+x] = transformCellToG16TrueValue(pixels[_y][_x], offset);
                }
            }
        },
        [&buf,offset, startX, endX, startY, width](std::vector<double>&& pixels, uint32_t row) {
            for (auto i = 0; i < pixels.size(); ++i) {
                buf[(row-startY)*width+i] = transformCellToG16TrueValue(pixels[i], offset);
            }
        },
        [this](uint32_t percent) {
            emit sendProgress(percent);
        },
        startY, endY, startX, endX)) {
            emit sendProgressError();
            return {};
        }
    return buf;
}

std::unique_ptr<uint8_t[]> ImageConverter::CreateRGB_UserValues(const QString &path, const std::map<double, color>& colorValues, int startX, int startY, int endX, int endY)
{
    auto width = (endX-startX+1);
    auto height = (endY-startY+1);
    constexpr auto numberOfChannels = 4;
    auto numberOfPixels = width*height;
    auto buf = std::unique_ptr<uint8_t[]>(new uint8_t[numberOfChannels*numberOfPixels]);
    if (!Tiff::LoadTiff(path,
        [&buf,&colorValues, numberOfPixels, startX, endX, startY, endY, width](std::vector<std::vector<double>>&& pixels, unsigned int _startX, unsigned int _startY) {
            for (auto _y = 0; _y < pixels.size(); ++_y) {
                if (_startY+_y < startY || _startY+_y > endY) continue;
                for (auto _x = 0; _x < pixels[0].size(); ++_x) {
                    if (_startX+_x < startX || _startX+_x > endX) continue;
                    auto x = _startX+_x-startX;
                    auto y = _startY+_y-startY;
                    auto ar = transformCellToRGBUserValues(pixels[_y][_x], colorValues);
                    auto position = y*width+x;
                    buf[position] = ar[0];
                    buf[position+1*numberOfPixels] = ar[1];
                    buf[position+2*numberOfPixels] = ar[2];
                    buf[position+3*numberOfPixels] = ar[3];
                }
            }
        },
        [&buf,&colorValues, startX, endX, startY, numberOfPixels, width](std::vector<double>&& pixels, uint32_t row) {
            for (auto i = 0; i < pixels.size(); ++i) {
                auto ar = transformCellToRGBUserValues(pixels[i], colorValues);
                auto position = (row-startY)*width+i;
                buf[position] = ar[0];
                buf[position+1*numberOfPixels] = ar[1];
                buf[position+2*numberOfPixels] = ar[2];
                buf[position+3*numberOfPixels] = ar[3];
            }
        },
        [this](uint32_t percent) {
            emit sendProgress(percent);
        },
        startY, endY, startX, endX)) {
            emit sendProgressError();
            return {};
        }
    return buf;
}

std::unique_ptr<unsigned char[]> ImageConverter::CreateRGB_UserRanges(const QString &path, const std::map<double, color>& colorValues, bool useGradient, int startX, int startY, int endX, int endY)
{
    auto width = (endX-startX+1);
    auto height = (endY-startY+1);
    constexpr auto numberOfChannels = 4;
    auto numberOfPixels = width*height;
    auto buf = std::unique_ptr<unsigned char[]>(new unsigned char[numberOfChannels*numberOfPixels]);
    if (!Tiff::LoadTiff(path,
        [&buf,&colorValues, numberOfPixels, useGradient, startX, endX, startY, endY, width](std::vector<std::vector<double>>&& pixels, unsigned int _startX, unsigned int _startY) {
            for (auto _y = 0; _y < pixels.size(); ++_y) {
                if (_startY+_y < startY || _startY+_y > endY) continue;
                for (auto _x = 0; _x < pixels[0].size(); ++_x) {
                    if (_startX+_x < startX || _startX+_x > endX) continue;
                    auto x = _startX+_x-startX;
                    auto y = _startY+_y-startY;
                    auto ar = transformCellToRGBUserRanges(pixels[_y][_x], colorValues, useGradient);
                    auto position = y*width+x;
                    buf[position] = ar[0];
                    buf[position+1*numberOfPixels] = ar[1];
                    buf[position+2*numberOfPixels] = ar[2];
                    buf[position+3*numberOfPixels] = ar[3];
                }
            }
        },
        [&buf,&colorValues, startX, endX, startY, numberOfPixels, useGradient, width](std::vector<double>&& pixels, uint32_t row) {
            for (auto i = 0; i < pixels.size(); ++i) {
                auto ar = transformCellToRGBUserRanges(pixels[i], colorValues, useGradient);
                auto position = (row-startY)*width+i;
                buf[position] = ar[0];
                buf[position+1*numberOfPixels] = ar[1];
                buf[position+2*numberOfPixels] = ar[2];
                buf[position+3*numberOfPixels] = ar[3];
            }
        },
        [this](uint32_t percent) {
            emit sendProgress(percent);
        },
        startY, endY, startX, endX)) {
            emit sendProgressError();
            return {};
        }
    return buf;
}

std::unique_ptr<unsigned char[]> ImageConverter::CreateRGB_Formula(const QString &path, int startX, int startY, int endX, int endY)
{
    auto width = (endX-startX+1);
    auto height = (endY-startY+1);
    constexpr auto numberOfChannels = 4;
    auto numberOfPixels = width*height;
    auto buf = std::unique_ptr<unsigned char[]>(new unsigned char[numberOfChannels*numberOfPixels]);

    if (!Tiff::LoadTiff(path,
        [&buf, numberOfPixels, startX, endX, startY, endY, width](std::vector<std::vector<double>>&& pixels, unsigned int _startX, unsigned int _startY) {
            for (auto _y = 0; _y < pixels.size(); ++_y) {
                if (_startY+_y < startY || _startY+_y > endY) continue;
                for (auto _x = 0; _x < pixels[0].size(); ++_x) {
                    if (_startX+_x < startX || _startX+_x > endX) continue;
                    auto x = _startX+_x-startX;
                    auto y = _startY+_y-startY;
                    auto position = y*width+x;
                    auto ar = transformCellToRGBFormula(pixels[_y][_x]);
                    buf[position] = ar[0];
                    buf[position+1*numberOfPixels] = ar[1];
                    buf[position+2*numberOfPixels] = ar[2];
                    buf[position+3*numberOfPixels] = ar[3];
                }
            }
        },
        [&buf, startX, endX, startY, numberOfPixels, width](std::vector<double>&& pixels, uint32_t row) {
            for (auto i = 0; i < pixels.size(); ++i) {
                auto ar = ImageConverter::transformCellToRGBFormula(pixels[i]);
                auto position = (row-startY)*width+i;
                buf[position] = ar[0];
                buf[position+1*numberOfPixels] = ar[1];
                buf[position+2*numberOfPixels] = ar[2];
                buf[position+3*numberOfPixels] = ar[3];
            }
        },
        [this](uint32_t percent) {
            emit sendProgress(percent);

        },
        startY, endY, startX, endX)) {
            emit sendProgressError();
            return {};
        }
    return buf;
}

std::unique_ptr<uint8_t[]> ImageConverter::CreateRGB_Points(const CsvConvertParams &params)
{
    constexpr auto numberOfChannels = 4;
    auto numberOfPixels = params.width*params.height;

    auto buf = std::unique_ptr<uint8_t[]>(new uint8_t[numberOfChannels*numberOfPixels]());

    auto csv = getRows(params.inputPath.toStdString());
    bool isAValidCoordinateFile = true;
    auto _boundaries = getBoundaries(csv, params.coordinateIndexes, isAValidCoordinateFile); // this is run even if the boundaries are set by user in order to check for invalid csv file
    if (!isAValidCoordinateFile) {
        Gui::ThrowError("Invalid csv file (columns at " + QString::number(params.coordinateIndexes[0]) + " and " + QString::number(params.coordinateIndexes[1]) + " are not numbers)");
        return buf;
    }
    auto boundaries = params.boundaries.value_or(_boundaries);

    auto threadCount = std::thread::hardware_concurrency();
    std::vector<std::thread> threads; threads.reserve(threadCount);
    for (auto i = 0; i < threadCount; ++i) {
        threads.emplace_back([i, threadCount, numberOfPixels, &csv, &buf, &params, &boundaries, this]() {
            size_t threadBegin = (float)i/threadCount*csv.size();
            size_t threadEnd = (float)(i+1)/threadCount*csv.size();
            for (auto row = threadBegin; row < threadEnd; ++row) {
                auto x = std::stod(csv[row][params.coordinateIndexes[0]]);
                auto y = std::stod(csv[row][params.coordinateIndexes[1]]);
                if (x < boundaries.minX || x > boundaries.maxX || y < boundaries.minY || y > boundaries.maxY) continue;
                int32_t centerX = std::round(Util::Remap(x, boundaries.minX, boundaries.maxX, 0, params.width-1));
                int32_t centerY = std::round(Util::Remap(y, boundaries.minY, boundaries.maxY, 0, params.height-1));
                size_t pos = centerY*params.width+centerX;

                auto styleIndex = getStyleForCsvShape(csv[row], params);
                auto centerColor = params.colors[styleIndex].second;
                buf[pos] = centerColor[0];
                buf[pos+1*numberOfPixels] = centerColor[1];
                buf[pos+2*numberOfPixels] = centerColor[2];
                buf[pos+3*numberOfPixels] = centerColor[3];

                auto shapeSize = params.shapes[styleIndex].second;
                if (shapeSize < 2) continue;
                int lowerBoundX = centerX-shapeSize+1, upperBoundX = centerX+shapeSize-1, lowerBoundY = centerY-shapeSize+1, upperBoundY = centerY+shapeSize-1;
                for (auto posX = lowerBoundX; posX <= upperBoundX; ++posX) {
                    for (auto posY = lowerBoundY; posY <= upperBoundY; ++posY) {
                        if (posX < 0 || posX >= params.width || posY < 0 || posY >= params.height) continue; // out of bounds
                        int currPos = posY*params.width+posX;
                        if (currPos == pos) continue; // is center
                        if (buf[currPos] == centerColor[0] &&
                            buf[currPos+1*numberOfPixels] == centerColor[1] &&
                            buf[currPos+2*numberOfPixels] == centerColor[2] &&
                            buf[currPos+3*numberOfPixels] == centerColor[3]) continue;
                        // is some other center

                        bool doesShapeFillThisCell = true;
                        switch(params.shapes[styleIndex].first) {
                            case Util::CsvShapeType::EmptySquare: doesShapeFillThisCell = posX == lowerBoundX || posX == upperBoundX || posY == lowerBoundY || posY == upperBoundY; break;
                            case Util::CsvShapeType::Circle: doesShapeFillThisCell = std::sqrt(std::pow(posX-centerX,2)+std::pow(posY-centerY,2)) < static_cast<double>(shapeSize-0.6); break;
                            case Util::CsvShapeType::EmptyCircle: doesShapeFillThisCell = std::sqrt(std::pow(posX-centerX,2)+std::pow(posY-centerY,2)) >= static_cast<double>(shapeSize-1) && std::sqrt(std::pow(posX-centerX,2)+std::pow(posY-centerY,2)) < static_cast<double>(shapeSize-0.1); break;
                            default: break;
                        }
                        if (!doesShapeFillThisCell) continue;

                        auto shapeColor = params.colors[styleIndex].first;
                        buf[currPos] = shapeColor[0];
                        buf[currPos+1*numberOfPixels] = shapeColor[1];
                        buf[currPos+2*numberOfPixels] = shapeColor[2];
                        buf[currPos+3*numberOfPixels] = shapeColor[3];
                    }
                }
                if (i == 0) {
                    float br = (float)row+1;
                    float nz = (float)csv.size()/threadCount;
                    emit sendProgress(br/nz*100);
                }
            }
        });
    }
    for (auto& thread : threads) thread.join();
    return buf;
}


cimg_library::CImg<uint8_t> ImageConverter::CreateRGB_VectorShapes(GeoJsonConvertParams params, bool flipY)
{
    cimg_library::CImg<uint8_t> img(params.width, params.height, 1, 4);
    auto properties = std::vector<QJsonObject>();

    auto allShapes = getAllShapesFromJson(params.inputPath, params.boundaries, properties);
    emit sendProgressReset("Creating the image...");
    auto threadCount = std::thread::hardware_concurrency();
    std::vector<std::thread> threads; threads.reserve(threadCount);
    std::mutex mtx;
    for (auto i = 0; i < threadCount; ++i) {
        threads.emplace_back([i, threadCount, &mtx, &img, &allShapes, &params, &properties, this]() {
            size_t threadBegin = (float)i/threadCount*allShapes.size();
            size_t threadEnd = (float)(i+1)/threadCount*allShapes.size();
            for (auto index = threadBegin; index < threadEnd; ++index) {
                if (i == 0) {
                    float br = (float)index+1;
                    float nz = (float)allShapes.size()/threadCount;
                    emit sendProgress(br/nz*100);
                }
                auto shape = allShapes[index].get();
                auto color = getColorForVectorShape(shape, params, properties);
                std::lock_guard lk (mtx);
                shape->drawShape(&img, color, params);
            }
        });
    }
    for (auto& thread : threads) thread.join();
    if (flipY) img.mirror('y');
    return img;
}

cimg_library::CImg<uint8_t> ImageConverter::CreateRGB_GeoPackage(GeoPackageConvertParams params, bool flipY)
{
    std::vector<std::vector<std::unique_ptr<Shape::Shape>>> allShapes(params.selectedLayers.size());
    std::vector<std::vector<color>> allColors(params.selectedLayers.size());
    boolean calculateBoundaries = !params.boundaries.has_value();
    size_t totalNumberOfShapes = 0;

    for (auto i = 0; i < params.selectedLayers.size(); ++i) {
        emit sendProgressReset("Processing " + QString::fromStdString(params.selectedLayers[i]) + "...");
        allShapes[i] = getAllShapesFromLayer(params.inputPath, params.selectedLayers[i], params, allColors[i], calculateBoundaries);
        totalNumberOfShapes += allShapes[i].size();
    }

    cimg_library::CImg<uint8_t> img(params.width, params.height, 1, 4);
    emit sendProgressReset("Creating the image...");
    size_t counter = 1;
    for (auto layer = 0; layer < params.selectedLayers.size(); ++layer) {
        for (auto index = 0; index < allShapes[layer].size(); ++index) {
            allShapes[layer][index]->drawShape(&img, allColors[layer][index], params.boundaries.value(), params.width, params.height);
            float br = (float)counter++;
            float nz = (float)totalNumberOfShapes;
            emit sendProgress(br/nz*100);
        }
    }
    if (flipY) img.mirror('y');
    return img;
}

std::vector<std::unique_ptr<Shape::Shape> > ImageConverter::getAllShapesFromLayer(const QString &path, std::string layerName, GeoPackageConvertParams& params,
                                                                                  std::vector<color>& outputColors, boolean boundariesNotSet)
{
    try {
        sqlite::database db(path.toStdString());

        Util::Boundaries bounds;
        if (boundariesNotSet) bounds = {std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
        else bounds = params.boundaries.value();

        std::string geomColumn;
        db << "select column_name from gpkg_geometry_columns where table_name is ?;" << layerName >> geomColumn;

        std::vector<std::string> allColumns;
        int geomColumnIndex = -1;
        auto counter = 0;
        db << "select name from pragma_table_info(?);" << layerName >> [&allColumns, &geomColumnIndex, &counter, geomColumn](std::string name) {
            allColumns.push_back(name);
            if (name == geomColumn) geomColumnIndex = counter;
            ++counter;
        };
        if (geomColumnIndex == -1) throw std::invalid_argument("Invalid GeoPackage file");
        auto columnCount = allColumns.size();

        size_t entryCount;
        std::string request = "select count(*) from " + layerName;
        db << request >> entryCount;
        if (entryCount == 0) return {};

        auto threadCount = std::thread::hardware_concurrency();
        std::vector<std::thread> threads; threads.reserve(threadCount);
        std::mutex mtx;

        std::vector<std::vector<std::string>> properties(entryCount);
        std::vector<std::vector<color>> colors(entryCount);
        std::vector<std::vector<uint8_t>> blobs(entryCount);
        std::vector<std::vector<std::unique_ptr<Shape::Shape>>> shapes(entryCount);

        std::string request2 = "select * from " + layerName;
        auto rows = db << request2;

        counter = 0;
        for (auto&& row : rows) {
            properties[counter] = std::vector<std::string>(columnCount);
            for (auto c = 0; c < geomColumnIndex; ++c) {
                row >> properties[counter][c];
            }
            row >> blobs[counter];
            for (auto c = geomColumnIndex+1; c < columnCount; ++c) {
                row >> properties[counter][c];
            }
            ++counter;
        }

        for (auto i = 0; i < threadCount; ++i) {
            threads.emplace_back([this, i, threadCount, &mtx, layerName, boundariesNotSet, entryCount, &bounds, &blobs, &shapes, &properties, &allColumns, &params, &colors]() {
                size_t threadBegin = (float)i/threadCount*entryCount;
                size_t threadEnd = (float)(i+1)/threadCount*entryCount;
                for (auto index = threadBegin; index < threadEnd; ++index) {

                    // GeoPackageBinaryHeader
                    bool littleEndianForHeader;
                    int envelopeSize;

                    // bytes 0 and 1 must be "GP"
                    if (!(blobs[index][0] == Gpkg::magic_1 && blobs[index][1] == Gpkg::magic_2)) throw std::invalid_argument(layerName + ": Geometry invalid at row " + std::to_string(index));
                    // byte 2 is GP version, irrelevant to the program

                    // byte 3 indicates flags
                    {
                        // 7 6 5 4 3 2 1 0
                        if ((blobs[index][3] & 0b00010000) != 0) continue; // bit 4 indicates empty geometry
                        switch(blobs[index][3] & 0b00001110) { // bits 3,2,1 indicate the type of the envelope
                            case Gpkg::envelope32: envelopeSize = 32; break;
                            case Gpkg::envelope48_1: case Gpkg::envelope48_2: envelopeSize = 48; break;
                            case Gpkg::envelope64: envelopeSize = 64; break;
                            default: throw std::invalid_argument(layerName + ": Invalid geometry header at row " + std::to_string(index));
                        }
                        littleEndianForHeader = (blobs[index][3] & 0b00000001) != 0; // bit 0 indicates endianness for the rest of the header
                    }

                    // the next 4 bytes indicate the id of the SRS, irrelevant to the program
                    // afterwards comes the envelope (bounding box)
                    if (boundariesNotSet) {
                        Util::Boundaries geomBounds;
                        Util::copyBytesToVar(blobs[index],  8, &geomBounds.minX, littleEndianForHeader);
                        Util::copyBytesToVar(blobs[index], 16, &geomBounds.maxX, littleEndianForHeader);
                        Util::copyBytesToVar(blobs[index], 24, &geomBounds.minY, littleEndianForHeader);
                        Util::copyBytesToVar(blobs[index], 32, &geomBounds.maxY, littleEndianForHeader);
                        std::lock_guard lk (mtx);
                        bounds.minX = std::min(bounds.minX, geomBounds.minX);
                        bounds.maxX = std::max(bounds.maxX, geomBounds.maxX);
                        bounds.minY = std::min(bounds.minY, geomBounds.minY);
                        bounds.maxY = std::max(bounds.maxY, geomBounds.maxY);
                    }

                    // header ends, geometry begins
                    auto beginIndex = shapes[index].size();
                    auto numOfShapes = readWKBGeometry(std::move(blobs[index]), shapes[index], envelopeSize, index);
                    for (auto s = beginIndex; s < beginIndex+numOfShapes; ++s) {
                        colors[index].emplace_back(getColorForVectorShape(shapes[index][s].get(), params, properties[index], layerName, allColumns));
                    }

                    if (i == 0) {
                        float br = (float)index+1;
                        float nz = (float)entryCount/threadCount;
                        emit sendProgress(br/nz*100);
                    }

                }
            });
        }
        for (auto& thread : threads) thread.join();

        std::vector<std::unique_ptr<Shape::Shape>> rv;

        for (auto i = 0; i < entryCount; ++i) {
            for (auto j = 0; j < shapes[i].size(); ++j) {
                outputColors.push_back(std::move(colors[i][j]));
                rv.push_back(std::move(shapes[i][j]));
            }
        }

        params.boundaries = std::move(bounds);
        return rv;
    }
    catch(const sqlite::sqlite_exception& e) {
        Gui::ThrowError("SQL error " + QString::number(e.get_code()) + ": " + e.what() + " during " + QString::fromStdString(e.get_sql()));
        emit sendProgressError();
        return {};
    }
    catch(std::invalid_argument& e) {
        Gui::ThrowError(QString::fromStdString(e.what()));
        emit sendProgressError();
        return {};
    }
}

uint32_t ImageConverter::readWKBGeometry(std::vector<unsigned char> &&bytes, std::vector<std::unique_ptr<Shape::Shape>>& outShapes, int envelopeSize, int row)
{
    size_t pos = 8+envelopeSize;
    bool littleEndian = bytes[pos] != 0;
    uint32_t wkbType;
    Util::copyBytesToVar(bytes, pos+1, &wkbType, littleEndian);
    if (wkbType == Gpkg::wkbGeometryCollection || wkbType == Gpkg::wkbGeometryCollectionZ ||
        wkbType == Gpkg::wkbGeometryCollectionM || wkbType == Gpkg::wkbGeometryCollectionZM) {
        uint32_t numGeometries;
        Util::copyBytesToVar(bytes, pos+1, &numGeometries, littleEndian);
        for (auto i = 0; i < numGeometries; ++i) {
            auto shape = Shape::fromWkbType(wkbType);
            shape->loadFromBlob(bytes, pos);
            shape->propertyId = row;
            outShapes.push_back(std::move(shape));
        }
        return numGeometries;
    }
    else {
        auto shape = Shape::fromWkbType(wkbType);
        shape->loadFromBlob(bytes, pos);
        shape->propertyId = row;
        outShapes.push_back(std::move(shape));
        return 1;
    }

}

std::vector<std::unique_ptr<Shape::Shape> > ImageConverter::getAllShapesFromJson(const QString& path, std::optional<Util::Boundaries>& boundaries, std::vector<QJsonObject>& outputProperties)
{
    auto rv = std::vector<std::unique_ptr<Shape::Shape>>();
    Util::Boundaries bounds;
    bool boundariesNotSet = !boundaries.has_value();
    if (boundariesNotSet) bounds = {std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(),std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()};
    else bounds = boundaries.value();

    try {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) throw std::invalid_argument("cannot open " + path.toStdString());
        auto bytes = file.readAll();

        QJsonParseError error;
        auto document = QJsonDocument::fromJson(bytes, &error);
        if (error.error != QJsonParseError::NoError) throw std::invalid_argument(error.errorString().toStdString());
        if (!document.isObject()) throw std::invalid_argument("document isn't an object");
        auto jsonObj = document.object();

        auto _features = jsonObj["features"];
        if (_features == QJsonValue::Undefined || !_features.isArray()) throw std::invalid_argument("invalid ""features"" object");
        auto features = _features.toArray();
        auto _properties = std::vector<QJsonObject>(features.size());

        auto threadCount = std::thread::hardware_concurrency();
        std::vector<std::thread> threads; threads.reserve(threadCount);
        std::mutex mtx;
        for (auto i = 0; i < threadCount; ++i) {
            threads.emplace_back([i, threadCount, &mtx, &rv, &features, &_properties, &bounds, boundariesNotSet, this]() {
                size_t threadBegin = (float)i/threadCount*features.size();
                size_t threadEnd = (float)(i+1)/threadCount*features.size();
                for (auto index = threadBegin; index < threadEnd; ++index) {
                    if (!features[index].isObject()) throw std::invalid_argument("found invalid object at index " + std::to_string(index));
                    auto featureObject = features[index].toObject();

                    auto properties = featureObject["properties"];
                    if (properties == QJsonValue::Undefined || !properties.isObject()) throw std::invalid_argument("invalid ""properties"" object at index " + std::to_string(index));
                    _properties[index] = properties.toObject();

                    if (featureObject.contains("geometries")) { // GeometryCollection

                        if (!featureObject.value("geometries").isArray()) throw std::invalid_argument("invalid ""geometries"" array at index " + std::to_string(index));
                        auto _geometries = featureObject["geometries"];
                        if (_geometries == QJsonValue::Undefined || !_geometries.isArray()) throw std::invalid_argument("invalid ""geometries"" array at index " + std::to_string(index));
                        auto geometries = _geometries.toArray();

                        for (const auto geometry : geometries) {
                            auto shape = getShapeFromJson(geometry);
                            if (shape == nullptr) throw std::invalid_argument("found invalid geometric shape at index " + std::to_string(index));
                            if (boundariesNotSet) {
                                auto geomBounds = shape->getBoundaries();
                                std::lock_guard lk (mtx);
                                bounds.minX = std::min(bounds.minX, geomBounds.minX);
                                bounds.maxX = std::max(bounds.maxX, geomBounds.maxX);
                                bounds.minY = std::min(bounds.minY, geomBounds.minY);
                                bounds.maxY = std::max(bounds.maxY, geomBounds.maxY);
                            }
                            shape->propertyId = index;
                            std::lock_guard lk (mtx);
                            rv.push_back(std::move(shape));
                        }
                    }

                    else if (featureObject.contains("geometry")) {
                        auto shape = getShapeFromJson(featureObject.value("geometry"));
                        if (shape == nullptr) throw std::invalid_argument("invalid geometric shape at index " + std::to_string(index));
                        if (boundariesNotSet) {
                            auto geomBounds = shape->getBoundaries();
                            std::lock_guard lk (mtx);
                            bounds.minX = std::min(bounds.minX, geomBounds.minX);
                            bounds.maxX = std::max(bounds.maxX, geomBounds.maxX);
                            bounds.minY = std::min(bounds.minY, geomBounds.minY);
                            bounds.maxY = std::max(bounds.maxY, geomBounds.maxY);
                        }
                        shape->propertyId = index;
                        std::lock_guard lk (mtx);
                        rv.push_back(std::move(shape));
                    }

                    else throw std::invalid_argument("object at index " + std::to_string(index) + " that doesn't contain geometry or geometries");

                    if (i == 0) {
                        float br = (float)index+1;
                        float nz = (float)features.size()/threadCount;
                        emit sendProgress(br/nz*100);
                    }
                }
            });
        }
        for (auto& thread : threads) thread.join();
        outputProperties = _properties;
        boundaries = bounds;

    } catch (std::invalid_argument e) {
        Gui::ThrowError("Error reading GeoJson: " + QString(e.what()));
        return {};
    }

    return rv;
}


bool ImageConverter::GetMinAndMaxValues(const QString& path, double& min, double& max, int startX, int endX, int startY, int endY)
{
    auto _min = std::numeric_limits<double>::max();
    auto _max = std::numeric_limits<double>::lowest();

    auto widthAndHeight = Tiff::GetWidthAndHeight(path);
    if (widthAndHeight.first == 0 || widthAndHeight.second == 0) return false;
    if (endX == -1) endX = widthAndHeight.first-1;
    if (endY == -1) endY = widthAndHeight.second-1;

    if (!Tiff::LoadTiff(path,
        [&_min,&_max](std::vector<std::vector<double>>&& pixels, unsigned int, unsigned int) {
            for (auto& row : pixels) {
                for (auto pixel : row) {
                    if (pixel < _min) _min = pixel;
                    if (pixel > _max) _max = pixel;
                }
            }
        },
        [&_min,&_max](std::vector<double>&& pixels, uint32_t row) {
            for (auto v : pixels) {
                if (v < _min) _min = v;
                if (v > _max) _max = v;
            }
        },
        [this](uint32_t percent) {
            emit sendProgress(percent);
        },
        startY, endY, startX, endX)) {
        emit sendProgressError();
        return false;
    }
    min = _min;
    max = _max;
    return true;
}

std::set<double> ImageConverter::GetDistinctValues(const QString &path)
{
    std::set<double> rv;
    bool ok = Tiff::LoadTiff(path,
        [&rv](std::vector<std::vector<double>>&& pixels, unsigned int, unsigned int) {
            for (auto& row : pixels) {
                for (auto pixel : row) {
                    rv.insert(pixel);
                }
            }
        },
        [&rv](std::vector<double>&& pixels, uint32_t) {
            for (auto v : pixels) {
                rv.insert(v);
            }
    },
        [this](uint32_t percent) {
            emit sendProgress(percent);
        }
    );
    if (!ok) {
        emit sendProgressError();
        return {};
    }
    return rv;
}

std::unique_ptr<double[]> ImageConverter::GetRawImageValues(const QString &path, int startX, int endX, int startY, int endY)
{
    auto width = (endX-startX+1);
    auto height = (endY-startY+1);

    auto buf = std::unique_ptr<double[]>(new double[width*height]);

    if (!Tiff::LoadTiff(path, [&buf, startX, endX, startY, endY, width](std::vector<std::vector<double>>&& pixels, std::uint32_t _startX, std::uint32_t _startY) {
        for (auto _y = 0; _y < pixels.size(); ++_y) {
                if (_startY+_y < startY || _startY+_y > endY) continue;
            for (auto _x = 0; _x < pixels[0].size(); ++_x) {
                if (_startX+_x < startX || _startX+_x > endX) continue;
                buf[(_startY+_y-startY)*width+_startX+_x-startX] = pixels[_y][_x];
            }
        }
    },
    [&buf, startX, endX, startY, endY, width](std::vector<double>&& pixels, uint32_t row) {
        for (auto i = 0; i < pixels.size(); ++i) {
            buf[(row-startY)*width+i] = pixels[i];
        }
    },
    [this](uint32_t percent) {
        emit sendProgress(percent);
    },
    startY, endY, startX, endX)) {
        emit sendProgressError();
        return nullptr;
    }
    return buf;
}
