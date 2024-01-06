#include "geojsonwindow.h"
#include "qtfunctions.h"
#include "ui_geojsonwindow.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThreadPool>
#include <QFileInfo>
#include "imageconverter.h"
#include "pngfunctions.h"
#include "previewtask.h"

GeoJsonWindow::GeoJsonWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeoJsonWindow)
{
    ui->setupUi(this);
    configWindow = nullptr;
    ui->progressBar->setVisible(false);
    ui->label_progress->setVisible(false);
}

GeoJsonWindow::~GeoJsonWindow()
{
    delete configWindow;
    delete ui;
}

#define displayProgressBar(desc) Util::displayProgressBar(ui->progressBar, ui->label_progress, desc);
#define hideProgressBar() Util::hideProgressBar(ui->progressBar, ui->label_progress);

void GeoJsonWindow::receiveParams(const GeoJsonConvertParams &params)
{
    parameters = params;
    Util::changeSuccessState(ui->label_success, Util::SuccessStateColor::Green);
}

void GeoJsonWindow::receivePreviewRequest(const GeoJsonConvertParams &params)
{
    parameters = params;
    previewImage();
}

void GeoJsonWindow::on_pushButton_inputPath_clicked()
{
    ui->lineEdit_inputPath->setText(Gui::GetInputPath("Choose GeoJson file","GeoJson files (*.geojson *.json)"));
    if (!ui->lineEdit_inputPath->text().isEmpty()) ui->pushButton_inputPath->setEnabled(false);
}


void GeoJsonWindow::on_pushButton_configure_clicked()
{
    if (ui->lineEdit_inputPath->text().isEmpty()) {
        Gui::ThrowError("You must select the file first");
        return;
    }
    if (configWindow != nullptr) {
        if (configWindow->isVisible()) {
            Gui::ThrowError("ConfigureRGB already opened.");
            return;
        }
    }
    if (parameters.has_value()) this->configWindow = new ConfigureRGBForm(ui->lineEdit_inputPath->text(), Util::OutputMode::RGB_Vector, parameters.value());
    else this->configWindow = new ConfigureRGBForm(ui->lineEdit_inputPath->text(), Util::OutputMode::RGB_Vector);
    connect(configWindow,SIGNAL(sendGeoJsonParams(const GeoJsonConvertParams&)),this,SLOT(receiveParams(const GeoJsonConvertParams&)));
    connect(configWindow,SIGNAL(sendPreviewRequest(const GeoJsonConvertParams&)),this,SLOT(receivePreviewRequest(const GeoJsonConvertParams&)));
    configWindow->show();
}

void GeoJsonWindow::on_pushButton_preview_clicked()
{
    previewImage();
}


void GeoJsonWindow::on_pushButton_reset_clicked()
{
    if (!Gui::GiveQuestion("Are you sure you want to reset your settings?")) {
        return;
    }
    if (configWindow != nullptr) {
        configWindow->close();
        configWindow = nullptr;
    }
    ui->lineEdit_inputPath->clear();
    ui->pushButton_inputPath->setEnabled(true);
    ui->lineEdit_width->setValue(1);
    ui->lineEdit_height->setValue(1);
    ui->doubleSpinBox_startX->setValue(0);
    ui->doubleSpinBox_startY->setValue(0);
    ui->doubleSpinBox_endX->setValue(0);
    ui->doubleSpinBox_endY->setValue(0);
    parameters.reset();
    Util::changeSuccessState(ui->label_success, Util::SuccessStateColor::Red);
}


void GeoJsonWindow::on_pushButton_save_clicked()
{
    if (!checkInput()) return;
    auto savePath = Gui::GetSavePath();
    if (savePath.isEmpty()) return;
    if (savePath.right(4) != ".png") savePath += ".png";
    setParameters();

    displayProgressBar("Reading JSON...");
    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &GeoJsonWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &GeoJsonWindow::receiveProgressError, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressReset, this, &GeoJsonWindow::receiveProgressReset, Qt::DirectConnection);
    auto img = io.CreateRGB_VectorShapes(parameters.value());
    Png::SavePng(img, savePath);
    hideProgressBar();
}

void GeoJsonWindow::receiveProgressUpdate(uint32_t progress)
{
    ui->progressBar->setValue(progress);
}

void GeoJsonWindow::receiveProgressError()
{
    hideProgressBar();
}

void GeoJsonWindow::receiveProgressReset(QString desc)
{
    displayProgressBar(desc);
}

bool GeoJsonWindow::checkInput()
{
    if (!parameters.has_value()) {
        Gui::ThrowError("You must configure the image first");
        return false;
    }

    auto startX = ui->doubleSpinBox_startX->value();
    auto startY = ui->doubleSpinBox_startY->value();
    auto endX = ui->doubleSpinBox_endX->value();
    auto endY = ui->doubleSpinBox_endY->value();
    if (!(startX == 0 && startY == 0 && endX == 0 && endY == 0)) {
        if (startX >= endX || startY >= endY) {
            Gui::ThrowError("Invalid boundary coordinates");
            return false;
        }
    }

    return true;
}

void GeoJsonWindow::setParameters()
{
    auto startX = ui->doubleSpinBox_startX->value();
    auto startY = ui->doubleSpinBox_startY->value();
    auto endX = ui->doubleSpinBox_endX->value();
    auto endY = ui->doubleSpinBox_endY->value();
    if (startX == 0 && startY == 0 && endX == 0 && endY == 0) {
        parameters.value().boundaries = std::nullopt;
    }
    else parameters.value().boundaries = {startX, endX, startY, endY};

    parameters.value().inputPath = ui->lineEdit_inputPath->text();
    parameters.value().width = ui->lineEdit_width->value();
    parameters.value().height = ui->lineEdit_height->value();
}

void GeoJsonWindow::previewImage()
{
    if (!checkInput()) return;
    setParameters();
    displayProgressBar("Reading JSON...");
    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &GeoJsonWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &GeoJsonWindow::receiveProgressError, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressReset, this, &GeoJsonWindow::receiveProgressReset, Qt::DirectConnection);
    auto img = io.CreateRGB_VectorShapes(parameters.value());
    auto task = new PreviewTask<uint8_t>(img);
    QThreadPool::globalInstance()->start(task);
    hideProgressBar();
}

