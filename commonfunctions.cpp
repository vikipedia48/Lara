#include "commonfunctions.h"
#include "qtfunctions.h"
#include <rapidcsv.h>
#include <sstream>

Color Color::fromString(const QString &str, bool& ok)
{
    std::array<unsigned char,4> colorValue;
    try {
        auto val = 0, lastcomma = 0;
        auto commaCount = std::count(str.begin(),str.end(),',');
        if (!(commaCount == 2 || commaCount == 3)) throw std::invalid_argument("");
        for (auto i = 0;; ++i) {
            bool conversionCorrect;
            if (str[i] == ',') {
                uint32_t temp = str.mid(lastcomma,i-lastcomma).toUInt(&conversionCorrect);
                if (!conversionCorrect || temp > 255) throw std::invalid_argument("");
                colorValue[val++] = (unsigned char)temp;
                if (!conversionCorrect) throw std::invalid_argument("");
                lastcomma = i+1;
                if (val == commaCount) {
                    uint32_t temp = str.mid(lastcomma).toUInt(&conversionCorrect);
                    if (!conversionCorrect || temp > 255) throw std::invalid_argument("");
                    colorValue[commaCount] = (unsigned char) temp;
                    if (commaCount == 2) colorValue[3] = 255;
                    break;
                }
            }
        }
        ok = true;
        return Color(colorValue);
    } catch (std::invalid_argument e) {
        ok = false;
        return Color();
    }
}

Color::operator QString() const
{
    return QString::number(r)+","+QString::number(g)+","+QString::number(b)+","+QString::number(a);
}

double Util::Remap(double value, double from1, double to1, double from2, double to2) {
    return (value - from1) / (to1 - from1) * (to2 - from2) + from2;
}

void Util::changeSuccessState(QLabel *label, SuccessStateColor color)
{
    if (color == Util::SuccessStateColor::Yellow) {
        label->setStyleSheet("background-color: rgb(237, 212, 0);color: rgb(255, 255, 255);margin-left:25%;border-radius:6%;");
        label->setText(" ! ");
    }
    else if (color == Util::SuccessStateColor::Red) {
        label->setStyleSheet("background-color: rgb(204, 0, 0);color: rgb(255, 255, 255);margin-left:25%;border-radius:6%;");
        label->setText(" ! ");
    }
    else if (color == Util::SuccessStateColor::Green) {
        label->setStyleSheet("background-color: rgb(78, 155, 6);color: rgb(255, 255, 255);margin-left:25%;border-radius:6%;");
        label->setText(" ✓ ");
    }
}

color Util::stringToColor(QString str, bool &ok)
{
    std::array<unsigned char,4> colorValue;
    try {
        auto val = 0, lastcomma = 0;
        auto commaCount = std::count(str.begin(),str.end(),',');
        if (!(commaCount == 2 || commaCount == 3)) throw std::invalid_argument("");
        for (auto i = 0;; ++i) {
            bool conversionCorrect;
            if (str[i] == ',') {
                uint32_t temp = str.mid(lastcomma,i-lastcomma).toUInt(&conversionCorrect);
                if (!conversionCorrect || temp > 255) throw std::invalid_argument("");
                colorValue[val++] = (unsigned char)temp;
                if (!conversionCorrect) throw std::invalid_argument("");
                lastcomma = i+1;
                if (val == commaCount) {
                    uint32_t temp = str.mid(lastcomma).toUInt(&conversionCorrect);
                    if (!conversionCorrect || temp > 255) throw std::invalid_argument("");
                    colorValue[commaCount] = (unsigned char) temp;
                    if (commaCount == 2) colorValue[3] = 255;
                    break;
                }
            }
        }
        ok = true;
        return colorValue;
    } catch (std::invalid_argument e) {
        ok = false;
        return {};
    }
}

Util::CsvShapeType Util::csvShapeTypeFromString(const QString &str)
{
    if (str.toUpper() == "SQUARE") return Util::CsvShapeType::Square;
    if (str.toUpper() == "EMPTYSQUARE") return Util::CsvShapeType::EmptySquare;
    if (str.toUpper() == "CIRCLE") return Util::CsvShapeType::Circle;
    if (str.toUpper() == "EMPTYCIRCLE") return Util::CsvShapeType::EmptyCircle;
    return Util::CsvShapeType::Error;
}

void Util::displayProgressBar(QProgressBar* bar, QLabel* label, QString desc)
{
    bar->show();
    bar->setMinimum(0);
    bar->setMaximum(100);
    bar->setValue(0);
    bar->setVisible(true);
    label->setVisible(true);
    label->setText(desc);
}

void Util::hideProgressBar(QProgressBar* bar, QLabel* label)
{
    bar->hide();
    label->hide();
}


Util::Profiler::Profiler(const char* name) noexcept : name(name), start{std::chrono::steady_clock::now()} {}
Util::Profiler::~Profiler() {
    const auto now = std::chrono::steady_clock::now();
    qDebug() << "time for " << name << " is " << std::chrono::duration_cast<std::chrono::microseconds>(now - start).count() << "us\n";
}

Util::GpkgLayerType Util::gpkgLayerTypeFromString(const std::string &str)
{
    if (str == "features") return GpkgLayerType::Features;
    if (str == "tiles") return GpkgLayerType::Tiles;
    if (str == "attributes") return GpkgLayerType::Attributes;
    return GpkgLayerType::Error;
}

QString Util::colorToString(const std::array<uint8_t, 4> color)
{
    return QString::number(color[0])+","+QString::number(color[1])+","+QString::number(color[2])+","+QString::number(color[3]);
}

QString Util::csvShapeTypeToString(CsvShapeType type)
{
    switch (type) {
        case CsvShapeType::Circle: return "Circle";
        case CsvShapeType::EmptyCircle: return "EmptyCircle";
        case CsvShapeType::Square: return "Square";
        case CsvShapeType::EmptySquare: return "EmptySquare";
        default: return "";
    }
}

std::vector<std::string> Util::getAllCsvColumns(const std::string& path)
{
    auto rv = std::vector<std::string>();
    rapidcsv::Document doc(path);
    rv = doc.GetColumnNames();
    if (rv.empty()) Gui::ThrowError("Invalid csv file");
    return rv;
}

