#ifndef NEWCSVWINDOW_H
#define NEWCSVWINDOW_H

#include "conversionparameters.h"
#include "luacodewindow.h"
#include <QLineEdit>
#include <QWidget>

namespace Ui {
class NewCsvWindow;
}

class NewCsvWindow : public QWidget
{
    Q_OBJECT

public:
    explicit NewCsvWindow(QWidget *parent = nullptr);
    ~NewCsvWindow();

private slots:
    void on_pushButton_inputPath_clicked();

    void on_pushButton_configure_clicked();

    void on_pushButton_preview_clicked();

    void on_pushButton_reset_clicked();

    void on_pushButton_save_clicked();

    void receiveProgressUpdate(uint32_t progress);
    void receiveProgressError();
    void receivePreviewRequest(const std::string& script);
    void receiveLuaScript(const std::string& script);

private:
    Ui::NewCsvWindow *ui;
    LuaCodeWindow* luaCodeWindow;
    NewCsvConvertParams params;

    bool checkIfEmpty(QLineEdit* lineEdit, bool printError = false);
    bool checkInput();
    void setParameters();
    void previewImage();
    void exportImage();
};

#endif // NEWCSVWINDOW_H
