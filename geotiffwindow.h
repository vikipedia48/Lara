#ifndef GEOTIFFWINDOW_H
#define GEOTIFFWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include "commonfunctions.h"
#include "configurergbform.h"
#include "conversionparameters.h"
#include "luacodewindow.h"

namespace Ui {
class GeotiffWindow;
}

class GeotiffWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GeotiffWindow(QWidget *parent = nullptr);
    GeotiffWindow(bool legacy, QWidget* parent = nullptr);
    ~GeotiffWindow();

private slots:
    void on_pushButton_inputFile_clicked();

    void on_comboBox_outputMode_activated(int index);

    void on_pushButton_outputModeConfigure_clicked();

    void on_comboBox_splitIntoTiles_activated(int index);

    void on_pushButton_reset_clicked();

    void on_pushButton_save_clicked();

    void on_pushButton_preview_clicked();

    void on_comboBox_scaleImage_activated(int index);

public slots:
    void receiveColorValues(const std::map<double,color>& colors);
    void receiveGradient(bool yes);
    void receivePreviewRequest(const std::map<double,color>& colorMap, bool gradient);
    void receivePreviewRequest(const std::map<double,color>& colorMap);
    void receivePreviewRequest(const std::string& luaScript);
    void receiveProgressUpdate(uint32_t progress);
    void receiveProgressError();
    void receiveProgressReset(QString desc);
    void receiveLuaScript(const std::string& code);

private:
    Ui::GeotiffWindow *ui;
    ConfigureRGBForm* configWindow;
    LuaCodeWindow* luaCodeWindow;
    TiffConvertParams parameters;
    std::vector<Util::OutputMode> outputModes;

    bool checkInput();
    void setParameters();
    void exportImage();
    void previewImage();
    void previewImage(const std::map<double,color>& colorMap, bool gradient);
    void previewImage(const std::map<double,color>& colorMap);
    void previewImage(const std::string& luaScript);
    void getTileSize(int& tileSizeX, int& tileSizeY, std::pair<int,int> widthAndHeight);

    Util::TileMode getTileModeSelected();
    Util::OutputMode getOutputModeSelected();
    Util::ScaleMode getScaleModeSelected();
};

#endif // GEOTIFFWINDOW_H
