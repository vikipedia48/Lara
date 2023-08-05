#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H

#include "boost/endian/detail/endian_load.hpp"
#include <vector>
#include <string>
#include <map>
#include <QString>
#include <QLabel>
#include <QProgressBar>

using color = std::array<unsigned char,4>;


struct Color {
    uint8_t r, g, b, a;
    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
    Color(std::array<unsigned char,4> ar) : r(ar[0]), g(ar[1]), b(ar[2]), a(ar[3]) {}
    static Color fromString(const QString& str, bool& ok);
    operator QString() const;
};

namespace Util {
template <typename T> static void copyBytesToVar(const std::vector<uint8_t>& bytes, size_t startPos, T* var, bool littleEndian = false) {

    auto size = sizeof(T);
    if (size == 8 && littleEndian) *var = boost::endian::endian_load<double, 8, boost::endian::order::little>(&bytes[startPos]);
    else if (size == 8 && !littleEndian) *var = boost::endian::endian_load<double, 8, boost::endian::order::big>(&bytes[startPos]);
    else if (size == 4 && littleEndian) *var = boost::endian::endian_load<uint32_t, 4, boost::endian::order::little>(&bytes[startPos]);
    else if (size == 4 && !littleEndian) *var = boost::endian::endian_load<uint32_t, 4, boost::endian::order::big>(&bytes[startPos]);
    else throw std::invalid_argument("unreachable code");
};


enum class SuccessStateColor {
    Red,
    Yellow,
    Green
};

enum class OutputMode {
    No,
    Grayscale16_TrueValue,
    Grayscale16_MinToMax,
    RGB_UserRanges,
    RGB_UserValues,
    RGB_Formula,
    RGB_Points,
    RGB_Vector,
    RGB_Gpkg
};

enum class TileMode {
    No,
    DefineByNumber,
    DefineBySize
};

enum class PixelSize {
    No,
    ThirtyTwoBit,
    SixteenBit
};

enum class CsvShapeType {
    Error,
    Square,
    EmptySquare,
    Circle,
    EmptyCircle
};

enum class ScaleMode {
    No,
    Decrease,
    Increase
};

enum class GpkgLayerType {
    Error,
    Features,
    Tiles,
    Attributes
};

double Remap(double value, double from1, double to1, double from2, double to2);
color stringToColor(QString str, bool& ok);
QString colorToString(const std::array<uint8_t,4> color);
void changeSuccessState(QLabel* label, SuccessStateColor color);
CsvShapeType csvShapeTypeFromString(const QString& str);
QString csvShapeTypeToString(CsvShapeType type);
void displayProgressBar(QProgressBar* bar, QLabel* label, QString desc);
void hideProgressBar(QProgressBar* bar, QLabel* label);
GpkgLayerType gpkgLayerTypeFromString(const std::string& str);

struct Boundaries {
    double minX, maxX, minY, maxY;
};

struct Profiler {
    const char* name;
    std::chrono::steady_clock::time_point start;

    Profiler(const char* name) noexcept;
    ~Profiler();
};

#define MEASURE($fn) \
    []() { \
        const auto _ = Util::Profiler(name); \
        $fn; \
    }

}


#endif // COMMONFUNCTIONS_H
