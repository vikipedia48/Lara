#include "qdebug.h"
#include "qtfunctions.h"
#include "tifffunctions.h"
#include "float.h"
#include <chrono>
#include <tiff.h>
#include <tiffio.h>
#include <cstdint>
#include <sol/sol.hpp>

std::pair<unsigned int, unsigned int> Tiff::GetWidthAndHeight(const QString &path)
{
    std::pair<unsigned int, unsigned int> rv = {0,0};
    TIFF* tif = TIFFOpen(path.toStdString().data(),"r");
    if (!tif) {
        Gui::ThrowError("Error loading image.");
        return rv;
    }
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH,&rv.first);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH,&rv.second);

    TIFFClose(tif);
    return rv;
}

bool Tiff::LoadTiff(const QString &path,
                    const Tiff::TileFunc_t& tileFunc, const StripFunc_t& stripFunc, const ProgressUpdateFunc_t& progressFunc,
                    int startY, int endY, int startX, int endX)
{
    TIFF* tif = TIFFOpen(path.toStdString().data(),"r");
    if (!tif) {
        Gui::ThrowError("Error loading image.");
        TIFFClose(tif);
        return false;
    }

    TiffProperties properties {};
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &properties.width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &properties.height);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &properties.bitsPerSample);
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &properties.sampleFormat);

    if (properties.width == 0 || properties.height == 0) {
        Gui::ThrowError("Error. Image is invalid.");
        TIFFClose(tif);
        return false;
    }
    if (!(properties.bitsPerSample == 8 || properties.bitsPerSample == 16 || properties.bitsPerSample == 32 ||
          properties.bitsPerSample == 64) || properties.sampleFormat > SAMPLEFORMAT_IEEEFP) {
        Gui::ThrowError("Unsupported file");
        TIFFClose(tif);
        return false;
    }
    if (endY == -1) endY = properties.height-1;
    if (endX == -1) endX = properties.width-1;

    auto threadCount = std::thread::hardware_concurrency();
    std::vector<std::thread> threads; threads.reserve(threadCount);
    std::mutex mtx;

    for (auto i = 0; i < threadCount; ++i) {
        threads.emplace_back([i, threadCount, &mtx, &tif, &properties, &tileFunc, &stripFunc, &progressFunc, startX, endX, startY, endY]() {
            void* buf;
            if (TIFFIsTiled(tif) != 0) {
                const auto tileSize = TIFFTileSize(tif);
                buf = _TIFFmalloc(tileSize);

                std::uint32_t tileWidth, tileHeight;
                TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth);
                TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileHeight);

                size_t totalNumberOfTiles_y = std::ceil((float)properties.height/tileHeight);
                size_t counter = 1;
                size_t threadBegin = (float)(i)/threadCount*totalNumberOfTiles_y*tileHeight;
                size_t threadEnd = (float)(i+1)/threadCount*totalNumberOfTiles_y*tileHeight;

                for (std::size_t currY = threadBegin; currY < threadEnd; currY += tileHeight) {
                    if (currY+tileHeight < startY || currY > endY) continue;
                    for (std::size_t currX = 0; currX < properties.width; currX += tileWidth) {
                        if (currX+tileWidth < startX || currX > endX) continue;
                        {
                            std::lock_guard lk (mtx);
                            TIFFReadTile(tif, buf, currX, currY, 0, 0);
                        }
                        auto pixels = GetVectorsFromTile(buf, properties, tileWidth, tileHeight);
                        tileFunc(std::move(pixels), currX, currY);
                    }
                    if (i == 0) {
                        float br = counter++;
                        float nz = (float)totalNumberOfTiles_y/threadCount;
                        progressFunc(br/nz*100);
                    }
                }
            }
            else {
                buf = _TIFFmalloc(TIFFScanlineSize(tif));
                auto counter = 1;
                size_t threadBegin = startY+(float)(i)/threadCount*(endY-startY+1);
                size_t threadEnd = startY+(float)(i+1)/threadCount*(endY-startY+1);

                for (auto row = threadBegin; row < threadEnd; ++row) {
                    if (row < startY || row > endY) continue;
                    {
                        std::lock_guard lk (mtx);
                        TIFFReadScanline(tif, buf, row);
                    }
                    auto pixels = GetVectorFromScanline(buf,properties, startX, endX);
                    stripFunc(std::move(pixels), row);
                    if (i == 0) {
                        float br = counter++;
                        float nz = (float)(endY-startY+1)/threadCount;
                        progressFunc(br/nz*100);
                    }
                }
            }
            _TIFFfree(buf);
        });
    }
    for (auto& thread : threads) thread.join();

    TIFFClose(tif);
    return true;
}

std::vector<double> Tiff::GetVectorFromScanline(void *data, const TiffProperties &properties, int startX, int endX)
{
    if (endX == -1) endX = properties.width-1;
    std::vector<double> rv(endX-startX+1);
        if (properties.sampleFormat == SAMPLEFORMAT_UINT) {
            if (properties.bitsPerSample == 8) {
                auto _data = (uint8_t*)data;
                auto counter = 0;
                for (auto i = startX; i <= endX; ++i) rv[counter++] = _data[i];
            }
            else if (properties.bitsPerSample == 16) {
                auto _data = (uint16_t*)data;
                auto counter = 0;
                for (auto i = startX; i <= endX; ++i) rv[counter++] = _data[i];
            }
            else if (properties.bitsPerSample == 32) {
                auto _data = (uint32_t*)data;
                auto counter = 0;
                for (auto i = startX; i <= endX; ++i) rv[counter++] = _data[i];
            }
            else if (properties.bitsPerSample == 64) {
                auto _data = (uint64_t*)data;
                auto counter = 0;
                for (auto i = startX; i <= endX; ++i) rv[counter++] = _data[i];
            }
        }
        else if (properties.sampleFormat == SAMPLEFORMAT_INT) {
            if (properties.bitsPerSample == 8) {
                auto _data = (int8_t*)data;
                auto counter = 0;
                for (auto i = startX; i <= endX; ++i) rv[counter++] = _data[i];
            }
            else if (properties.bitsPerSample == 16) {
                auto _data = (int16_t*)data;
                auto counter = 0;
                for (auto i = startX; i <= endX; ++i) rv[counter++] = _data[i];
            }
            else if (properties.bitsPerSample == 32) {
                auto _data = (int32_t*)data;
                auto counter = 0;
                for (auto i = startX; i <= endX; ++i) rv[counter++] = _data[i];
            }
            else if (properties.bitsPerSample == 64) {
                auto _data = (int64_t*)data;
                auto counter = 0;
                for (auto i = startX; i <= endX; ++i) rv[counter++] = _data[i];
            }
        }
        else if (properties.sampleFormat == SAMPLEFORMAT_IEEEFP) {
            if (properties.bitsPerSample == 32) {
                auto _data = (float*)data;
                auto counter = 0;
                for (auto i = startX; i <= endX; ++i) rv[counter++] = _data[i];
            }
            else if (properties.bitsPerSample == 64) {
                auto _data = (double*)data;
                auto counter = 0;
                for (auto i = startX; i <= endX; ++i) rv[counter++] = _data[i];
            }
        }
        return rv;
}

std::vector<std::vector<double>> Tiff::GetVectorsFromTile(void *data, const TiffProperties& properties, unsigned int tileWidth, unsigned int tileHeight)
{
    std::vector<std::vector<double>> rv;
    rv.reserve(tileHeight);

    if (properties.sampleFormat == SAMPLEFORMAT_UINT) {
        if (properties.bitsPerSample == 8) {
            auto _data = (uint8_t*)data;
            auto counter = 0;
            for (auto y = 0; y < tileHeight; ++y) {
                auto row = std::vector<double>(tileWidth);
                for (auto x = 0; x < tileWidth; ++x) {
                    row[x] = _data[counter++];
                }
                rv.push_back(row);
            }
        }
        else if (properties.bitsPerSample == 16) {
            auto _data = (uint16_t*)data;
            auto counter = 0;
            for (auto y = 0; y < tileHeight; ++y) {
                auto row = std::vector<double>(tileWidth);
                for (auto x = 0; x < tileWidth; ++x) {
                    row[x] = _data[counter++];
                }
                rv.push_back(row);
            }
        }
        else if (properties.bitsPerSample == 32) {
            auto _data = (uint32_t*)data;
            auto counter = 0;
            for (auto y = 0; y < tileHeight; ++y) {
                auto row = std::vector<double>(tileWidth);
                for (auto x = 0; x < tileWidth; ++x) {
                    row[x] = _data[counter++];
                }
                rv.push_back(row);
            }
        }
        else if (properties.bitsPerSample == 64) {
            auto _data = (uint64_t*)data;
            auto counter = 0;
            for (auto y = 0; y < tileHeight; ++y) {
                auto row = std::vector<double>(tileWidth);
                for (auto x = 0; x < tileWidth; ++x) {
                    row[x] = _data[counter++];
                }
                rv.push_back(row);
            }
        }
    }

    else if (properties.sampleFormat == SAMPLEFORMAT_INT) {
        if (properties.bitsPerSample == 8) {
            auto _data = (int8_t*)data;
            auto counter = 0;
            for (auto y = 0; y < tileHeight; ++y) {
                auto row = std::vector<double>(tileWidth);
                for (auto x = 0; x < tileWidth; ++x) {
                    row[x] = _data[counter++];
                }
                rv.push_back(row);
            }
        }
        else if (properties.bitsPerSample == 16) {
            auto _data = (int16_t*)data;
            auto counter = 0;
            for (auto y = 0; y < tileHeight; ++y) {
                auto row = std::vector<double>(tileWidth);
                for (auto x = 0; x < tileWidth; ++x) {
                    row[x] = _data[counter++];
                }
                rv.push_back(row);
            }
        }
        else if (properties.bitsPerSample == 32) {
            auto _data = (int32_t*)data;
            auto counter = 0;
            for (auto y = 0; y < tileHeight; ++y) {
                auto row = std::vector<double>(tileWidth);
                for (auto x = 0; x < tileWidth; ++x) {
                    row[x] = _data[counter++];
                }
                rv.push_back(row);
            }
        }
        else if (properties.bitsPerSample == 64) {
            auto _data = (int64_t*)data;
            auto counter = 0;
            for (auto y = 0; y < tileHeight; ++y) {
                auto row = std::vector<double>(tileWidth);
                for (auto x = 0; x < tileWidth; ++x) {
                    row[x] = _data[counter++];
                }
                rv.push_back(row);
            }
        }
    }
    else if (properties.sampleFormat == SAMPLEFORMAT_IEEEFP) {
        if (properties.bitsPerSample == 32) {
            auto _data = (float*)data;
            auto counter = 0;
            for (auto y = 0; y < tileHeight; ++y) {
                auto row = std::vector<double>(tileWidth);
                for (auto x = 0; x < tileWidth; ++x) {
                    row[x] = _data[counter++];
                }
                rv.push_back(row);
            }
        }
        else if (properties.bitsPerSample == 64) {
            auto _data = (double*)data;
            auto counter = 0;
            for (auto y = 0; y < tileHeight; ++y) {
                auto row = std::vector<double>(tileWidth);
                for (auto x = 0; x < tileWidth; ++x) {
                    row[x] = _data[counter++];
                }
                rv.push_back(row);
            }
        }
    }

    return rv;

}
