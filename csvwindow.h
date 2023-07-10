#ifndef CSVWINDOW_H
#define CSVWINDOW_H

#include "configurergbform.h"
#include "conversionparameters.h"

#include <QLineEdit>
#include <QWidget>

namespace Ui {
class CSVWindow;
}

class CSVWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CSVWindow(QWidget *parent = nullptr);
    ~CSVWindow();

public slots:
    void receiveCsvParameters(const CsvConvertParams& params);
    void receivePreviewRequest(const CsvConvertParams& params);

private slots:
    void on_pushButton_inputPath_clicked();

    void on_pushButton_configure_clicked();

    void on_pushButton_preview_clicked();

    void on_pushButton_reset_clicked();

    void on_pushButton_save_clicked();

    void receiveProgressUpdate(uint32_t progress);
    void receiveProgressError();

private:
    Ui::CSVWindow *ui;
    ConfigureRGBForm* configWindow;
    std::optional<CsvConvertParams> params;

    bool checkIfEmpty(QLineEdit* lineEdit, bool printError = false);
    bool checkInput();
    void setParameters();
    void previewImage();
    void exportImage();

};

#endif // CSVWINDOW_H
