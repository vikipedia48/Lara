#ifdef __linux__
#define cimg_display 1
#elif _WIN32
#define cimg_display 2
#else
#define cimg_display 0
#endif

#ifndef PNGFUNCTIONS_H
#define PNGFUNCTIONS_H

#define cimg_use_png
#include <memory>
#include <QString>
#include <QtDebug>
#include <map>
#include "commonfunctions.h"
#include <CImg.h>


namespace Png {
template<typename T>
cimg_library::CImg<T> CreatePngData(T* img, std::pair<uint32_t,uint32_t> widthAndHeight, Util::PixelSize pixelSize, bool flipY = false) {
    if (pixelSize == Util::PixelSize::ThirtyTwoBit) {
        cimg_library::CImg<uint8_t> rv(img, widthAndHeight.first, widthAndHeight.second, 1, 4);
        if (flipY) rv.mirror('y');
        return rv;
    }
    else if (pixelSize == Util::PixelSize::SixteenBit) {
        cimg_library::CImg<uint16_t> rv(img, widthAndHeight.first, widthAndHeight.second, 1, 1);
        if (flipY) rv.mirror('y');
        return rv;
    }
    else throw std::invalid_argument("unreachable code");
}
template<typename T>
void SavePng(cimg_library::CImg<T>& img, const QString& path) {
    img.save_png(path.toStdString().data());
}


}

#endif // PNGFUNCTIONS_H
