#ifndef GEOJSONWINDOW_H
#define GEOJSONWINDOW_H

#include "configurergbform.h"
#include "conversionparameters.h"

#include <QWidget>

namespace Ui {
class GeoJsonWindow;
}

class GeoJsonWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GeoJsonWindow(QWidget *parent = nullptr);
    ~GeoJsonWindow();

public slots:
    void receiveParams(const GeoJsonConvertParams& params);
    void receivePreviewRequest(const GeoJsonConvertParams& params);

private slots:
    void on_pushButton_inputPath_clicked();

    void on_pushButton_configure_clicked();

    void on_pushButton_preview_clicked();

    void on_pushButton_reset_clicked();

    void on_pushButton_save_clicked();

    void receiveProgressUpdate(uint32_t progress);
    void receiveProgressError();
    void receiveProgressReset(QString desc);

private:
    Ui::GeoJsonWindow *ui;
    ConfigureRGBForm* configWindow;
    std::optional<GeoJsonConvertParams> parameters;

    bool checkInput();
    void setParameters();
    void previewImage();
};

#endif // GEOJSONWINDOW_H
