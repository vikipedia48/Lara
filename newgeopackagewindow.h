#ifndef NEWGEOPACKAGEWINDOW_H
#define NEWGEOPACKAGEWINDOW_H

#include "conversionparameters.h"
#include "luacodewindow.h"

#include <QCheckBox>
#include <QWidget>

namespace Ui {
class NewGeoPackageWindow;
}

class NewGeoPackageWindow : public QWidget
{
    Q_OBJECT

public:
    explicit NewGeoPackageWindow(QWidget *parent = nullptr);
    ~NewGeoPackageWindow();

private slots:
    void on_pushButton_inputPath_clicked();

    void on_pushButton_configure_clicked();

    void on_pushButton_preview_clicked();

    void on_pushButton_reset_clicked();

    void on_pushButton_save_clicked();

    void receiveLuaScript(const std::string& luaScript);
    void receivePreviewRequest(const std::string& luaScript);
    void receiveProgressUpdate(uint32_t progress);
    void receiveProgressError();
    void receiveProgressReset(QString desc);

private:
    Ui::NewGeoPackageWindow *ui;
    LuaCodeWindow* luaCodeWindow;
    NewGeoPackageConvertParams parameters;

    std::vector<Util::GpkgLayer> layers;
    std::vector<QCheckBox*> layerChecks;

    bool loadLayers(const QString& path);
    bool checkInput();
    void setParameters();
    void previewImage();
};

#endif // NEWGEOPACKAGEWINDOW_H
