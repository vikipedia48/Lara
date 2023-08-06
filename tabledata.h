#ifndef TABLEDATA_H
#define TABLEDATA_H

#include "conversionparameters.h"

#include <QStringList>
#include <QTableWidget>



struct TableData {
    QStringList headers;
    std::vector<std::vector<QString>> cells;

    operator LayerParams() const;

    TableData() = default;
    TableData(QTableWidget* table);
    TableData(const TiffConvertParams& tiffParams);
    TableData(const CsvConvertParams& csvParams, const QStringList& headers);
    TableData(const GeoJsonConvertParams &jsonParams, const QStringList& headers);
    TableData(const QStringList& headers);

    void clear();
};

#endif // TABLEDATA_H
