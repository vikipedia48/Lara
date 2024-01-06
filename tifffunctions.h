#ifndef TIFFFUNCTIONS_H
#define TIFFFUNCTIONS_H

#include <vector>
#include <QString>
#include <queue>
#include <set>
#include <tiff.h>
#include <tiffio.h>
#include <memory>
#include <mutex>
#include <QThreadPool>
#include <QDebug>
#include <QProgressBar>


namespace Tiff {
struct TiffProperties {
    unsigned int width, height, bitsPerSample, sampleFormat;
};

using TileFunc_t = std::function<void (std::vector<std::vector<double>>&&, std::uint32_t, std::uint32_t)>;
using StripFunc_t = std::function<void (std::vector<double>&&, std::uint32_t)>;
using ProgressUpdateFunc_t = std::function<void (uint32_t)>;

std::pair<unsigned int, unsigned int> GetWidthAndHeight(const QString& path);
bool LoadTiff(const QString& path, const TileFunc_t& tileFunc, const StripFunc_t& stripFunc, const ProgressUpdateFunc_t& progressFunc, int startY = 0, int endY = -1, int startX = 0, int endX = -1);
//bool LoadTiffWithLua(const QString& path,  const std::string& luaFunc, int startY = 0, int endY = -1, int startX = 0, int endX = -1);
std::vector<double> GetVectorFromScanline(void* data, const TiffProperties& properties, int startX = 0, int endX = -1);
std::vector<std::vector<double>> GetVectorsFromTile(void* data, const TiffProperties& properties, unsigned int tileWidth, unsigned int tileHeight);

}

#endif // TIFFFUNCTIONS_H
