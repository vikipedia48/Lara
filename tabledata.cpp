#include "tabledata.h"

TableData::TableData(const QStringList& headers)
{
    this->headers = headers;
    cells = std::vector<std::vector<QString>>(1);
    cells[0] = std::vector<QString>(headers.size());
}

TableData::operator LayerParams() const
{
    LayerParams rv;

    bool isTableEmpty = true;
    for (auto& v : cells) {
        if (std::find_if(v.begin(), v.end(), [](QString str) { return !str.isEmpty(); }) != v.end()) {
            isTableEmpty = false;
            break;
        }
    }
    if (isTableEmpty) return rv;

    for (auto i = 0; i < cells.size(); ++i) {

        if (i != 0) {
            bool rowIsEmpty = true;
            for (auto& v : cells[i]) {
                if (!v.isEmpty()) {
                    rowIsEmpty = false;
                    break;
                }
            }
            if (rowIsEmpty) continue;
        }

        bool ok;
        auto color = Util::stringToColor(cells[i][0], ok);
        if (!ok) throw std::invalid_argument("Color at row " + std::to_string(i) + " is invalid");
        rv.colors.push_back(color);

        if (i == 0) {
            rv.columnValues.push_back({});
            continue;
        }
        auto it = std::find_if(cells[i].begin()+1, cells[i].end(), [](QString str) {
            return !str.isEmpty();
        });
        if (it == cells[i].end()) throw std::invalid_argument("Color at row " + std::to_string(i) + " isn't bound to any value");
        rv.columnValues.emplace_back(std::pair<std::string, std::string>{headers[it-cells[i].begin()].toStdString(), (*it).toStdString()});
    }

    return rv;
}

TableData::TableData(QTableWidget *table)
{
    headers = QStringList(table->columnCount());
    for (auto i = 0; i < headers.count(); ++i) {
        headers[i] = table->horizontalHeaderItem(i)->text();
    }
    cells = std::vector<std::vector<QString>>(table->rowCount());
    for (auto j = 0; j < cells.size(); ++j) {
        cells[j] = std::vector<QString>(table->columnCount());
        for (auto i = 0; i < table->columnCount(); ++i) cells[j][i] = table->item(j,i)->text();
    }
}

TableData::TableData(const TiffConvertParams &tiffParams)
{
    headers = {"Value","Color"};
    auto rowCount = tiffParams.colorValues.value().size();

    auto it = tiffParams.colorValues.value().begin();
    cells.emplace_back(std::vector<QString>(2));
    cells[0][0] = QString::number(it->first, 'g',17);
    cells[0][1] = Util::colorToString(it->second);
    if (rowCount == 1) return;

    auto last = std::prev(tiffParams.colorValues.value().end());
    cells.emplace_back(std::vector<QString>(2));
    cells[1][0] = QString::number(last->first,'g',17);
    cells[1][1] = Util::colorToString(last->second);
    if (rowCount == 2) return;

    for (it = ++it; it != last; ++it) {
        cells.emplace_back(std::vector<QString>{QString::number(it->first, 'g', 17), Util::colorToString(it->second)});
    }

}

TableData::TableData(const CsvConvertParams &csvParams, const QStringList& headers)
{
    this->headers = headers;
    auto rowCount = csvParams.columnValues.size();
    auto columnCount = this->headers.count();
    cells = std::vector<std::vector<QString>>(rowCount);
    for (auto r = 0; r < rowCount; ++r) {
        cells[r] = std::vector<QString>(columnCount);
        cells[r][0] = Util::colorToString(csvParams.colors[r].first);
        cells[r][1] = Util::colorToString(csvParams.colors[r].second);
        cells[r][2] = Util::csvShapeTypeToString(csvParams.shapes[r].first);
        cells[r][3] = QString::number(csvParams.shapes[r].second);
        if (r == 0) continue;
        cells[r][csvParams.columnValues[r].first+4] = csvParams.columnValues[r].second;
    }
}

TableData::TableData(const GeoJsonConvertParams &jsonParams, const QStringList& headers)
{
    this->headers = headers;
    auto rowCount = jsonParams.columnValues.size();
    auto columnCount = this->headers.count();
    cells = std::vector<std::vector<QString>>(rowCount);
    for (auto r = 0; r < rowCount; ++r) {
        cells[r] = std::vector<QString>(columnCount);
        cells[r][0] = Util::colorToString(jsonParams.colors[r]);
        if (r == 0) continue;
        auto index = std::find(this->headers.begin(), this->headers.end(), jsonParams.columnValues[r].first) - this->headers.begin();
        cells[r][index] = jsonParams.columnValues[r].second;
    }
}

