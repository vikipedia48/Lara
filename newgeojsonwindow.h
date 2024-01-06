#ifndef NEWGEOJSONWINDOW_H
#define NEWGEOJSONWINDOW_H

#include "conversionparameters.h"
#include "luacodewindow.h"

#include <QWidget>

namespace Ui {
class NewGeoJsonWindow;
}

class NewGeoJsonWindow : public QWidget
{
    Q_OBJECT

public:
    explicit NewGeoJsonWindow(QWidget *parent = nullptr);
    ~NewGeoJsonWindow();

private slots:
    void on_pushButton_inputPath_clicked();

    void on_pushButton_configure_clicked();

    void on_pushButton_preview_clicked();

    void on_pushButton_reset_clicked();

    void on_pushButton_save_clicked();

    void receiveProgressUpdate(uint32_t progress);
    void receiveProgressError();
    void receiveProgressReset(QString desc);
    void receiveLuaScript(const std::string& script);
    void receivePreviewRequest(const std::string& script);

private:
    Ui::NewGeoJsonWindow *ui;
    LuaCodeWindow* luaCodeWindow;
    NewGeoJsonConvertParams parameters;

    bool checkInput();
    void setParameters();
    void previewImage();
};

#endif // NEWGEOJSONWINDOW_H
