#ifndef CONFIGURERGBFORM_H
#define CONFIGURERGBFORM_H

#include <QWidget>
#include "commonfunctions.h"
#include "conversionparameters.h"

using color = std::array<unsigned char,4>;

namespace Ui {
class ConfigureRGBForm;
}

class ConfigureRGBForm : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigureRGBForm(QWidget *parent = nullptr);
    ConfigureRGBForm(const QString& inputPath, Util::OutputMode mode, QWidget *parent = nullptr);
    ~ConfigureRGBForm();
    Util::OutputMode mode;

private slots:
    void on_pushButton_clear_clicked();

    void on_pushButton_ok_clicked();

    void on_pushButton_cancel_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_preview_clicked();

    void on_tableWidget_cellClicked(int row, int column);

    void receiveProgressUpdate(uint32_t progress);
    void receiveProgressError();

signals:
    void sendColorValues(const std::map<double,color>& colors);
    void sendGradient(bool yes);
    void sendMinAndMax(std::array<double,2>& minAndMax);
    void sendPreviewRequest(const std::map<double,color>& colors, bool gradient);
    void sendPreviewRequest(const std::map<double,color>& colors);
    void sendPreviewRequest(const CsvConvertParams& params);
    void sendGeoJsonParams(const GeoJsonConvertParams& params);
    void sendCsvParams(const CsvConvertParams& params);
    void sendPreviewRequest(const GeoJsonConvertParams& params);
private:
    Ui::ConfigureRGBForm *ui;

    std::pair<double,color> getRowValues(int row, bool& ok);
    void addNewRowToTable(QString firstValue = "", QString secondValue = "", bool isFirstColumnEditable = true);
    void addNewRowToTable(Util::OutputMode pointsMode);
    bool isRowEmpty(int row);
    std::map<double,color> readTable();
    CsvConvertParams prepareCsvParams(bool& ok);
    GeoJsonConvertParams prepareGeoJsonParams(bool& ok);
    bool getPropertiesFromGeoJson(const QString& path, QJsonObject& outputProperties);
    std::vector<std::string> getAllCsvColumns(const std::string& path);
};

#endif // CONFIGURERGBFORM_H
