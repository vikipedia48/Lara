#include "commonfunctions.h"
#include <sstream>

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
                colorValue[val++] = (unsigned char)str.mid(lastcomma,i-lastcomma).toUInt(&conversionCorrect);
                if (!conversionCorrect) throw std::invalid_argument("");
                lastcomma = i+1;
                if (val == commaCount) {
                    colorValue[commaCount] = (unsigned char) str.mid(lastcomma).toUInt(&conversionCorrect);
                    if (!conversionCorrect) throw std::invalid_argument("");
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
    if (str == "Square") return Util::CsvShapeType::Square;
    if (str == "EmptySquare") return Util::CsvShapeType::EmptySquare;
    if (str == "Circle") return Util::CsvShapeType::Circle;
    if (str == "EmptyCircle") return Util::CsvShapeType::EmptyCircle;
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
