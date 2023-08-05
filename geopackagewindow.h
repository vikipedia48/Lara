#ifndef GEOPACKAGEWINDOW_H
#define GEOPACKAGEWINDOW_H

#include "configurergbform.h"
#include "conversionparameters.h"

#include <QCheckBox>
#include <QWidget>

namespace Ui {
class GeoPackageWindow;
}

class GeoPackageWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GeoPackageWindow(QWidget *parent = nullptr);
    ~GeoPackageWindow();

private slots:
    void on_pushButton_inputPath_clicked();

    void on_pushButton_configure_clicked();

    void on_pushButton_preview_clicked();

    void on_pushButton_reset_clicked();

    void on_pushButton_save_clicked();

    void receiveParams(const GeoPackageConvertParams& params);
    void receivePreviewRequest(const GeoPackageConvertParams& params);
    void receiveProgressUpdate(uint32_t progress);
    void receiveProgressError();
    void receiveProgressReset(QString desc);

private:
    struct GpkgLayer {
            Util::GpkgLayerType dataType;
            std::string tableName;
            std::string identifier;
        };

    Ui::GeoPackageWindow *ui;
    ConfigureRGBForm* configWindow;
    GeoPackageConvertParams parameters;

    std::vector<GpkgLayer> layers;
    std::vector<QCheckBox*> layerChecks;

    bool loadLayers(const QString& path);
    bool checkInput();
    void setParameters();
    void previewImage();
};

#endif // GEOPACKAGEWINDOW_H
