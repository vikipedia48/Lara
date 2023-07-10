#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H

#include <vector>
#include <string>
#include <map>
#include <QString>
#include <QLabel>
#include <QProgressBar>

using color = std::array<unsigned char,4>;

namespace Util {

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
    RGB_Vector
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

double Remap(double value, double from1, double to1, double from2, double to2);
color stringToColor(QString str, bool& ok);
void changeSuccessState(QLabel* label, SuccessStateColor color);
CsvShapeType csvShapeTypeFromString(const QString& str);
void displayProgressBar(QProgressBar* bar, QLabel* label, QString desc);
void hideProgressBar(QProgressBar* bar, QLabel* label);

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
