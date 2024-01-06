#include "imageconverter.h"
#include "newgeojsonwindow.h"
#include "qtfunctions.h"
#include "ui_newgeojsonwindow.h"
#include "pngfunctions.h"
#include "previewtask.h"

#include <QThreadPool>

NewGeoJsonWindow::NewGeoJsonWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NewGeoJsonWindow)
{
    ui->setupUi(this);
    luaCodeWindow = nullptr;
    ui->progressBar->setVisible(false);
    ui->label_progress->setVisible(false);
}

NewGeoJsonWindow::~NewGeoJsonWindow()
{
    delete luaCodeWindow;
    delete ui;
}

#define displayProgressBar(desc) Util::displayProgressBar(ui->progressBar, ui->label_progress, desc);
#define hideProgressBar() Util::hideProgressBar(ui->progressBar, ui->label_progress);


void NewGeoJsonWindow::on_pushButton_inputPath_clicked()
{
    ui->lineEdit_inputPath->setText(Gui::GetInputPath("Choose GeoJson file","GeoJson files (*.geojson *.json)"));
    if (!ui->lineEdit_inputPath->text().isEmpty()) ui->pushButton_inputPath->setEnabled(false);
}


void NewGeoJsonWindow::on_pushButton_configure_clicked()
{
    if (ui->lineEdit_inputPath->text().isEmpty()) {
        Gui::ThrowError("You must select the file first");
        return;
    }
    if (luaCodeWindow != nullptr) {
        if (luaCodeWindow->isVisible()) {
            Gui::ThrowError("ConfigureRGB already opened.");
            return;
        }
    }
    luaCodeWindow = new LuaCodeWindow(Util::OutputMode::RGB_Vector, parameters.luaScript.value_or(""));
    connect(luaCodeWindow,&LuaCodeWindow::sendCode,this,&NewGeoJsonWindow::receiveLuaScript);
    connect(luaCodeWindow,&LuaCodeWindow::sendPreviewRequest,this,&NewGeoJsonWindow::receivePreviewRequest);
    luaCodeWindow->show();
}


void NewGeoJsonWindow::on_pushButton_preview_clicked()
{
    previewImage();
}


void NewGeoJsonWindow::on_pushButton_reset_clicked()
{
    if (!Gui::GiveQuestion("Are you sure you want to reset your settings?")) {
        return;
    }
    if (luaCodeWindow != nullptr) {
        luaCodeWindow->close();
        luaCodeWindow = nullptr;
    }
    ui->lineEdit_inputPath->clear();
    ui->pushButton_inputPath->setEnabled(true);
    ui->lineEdit_width->setValue(1);
    ui->lineEdit_height->setValue(1);
    ui->doubleSpinBox_startX->setValue(0);
    ui->doubleSpinBox_startY->setValue(0);
    ui->doubleSpinBox_endX->setValue(0);
    ui->doubleSpinBox_endY->setValue(0);
    parameters = NewGeoJsonConvertParams();
    Util::changeSuccessState(ui->label_success, Util::SuccessStateColor::Red);
}


void NewGeoJsonWindow::on_pushButton_save_clicked()
{
    if (!checkInput()) return;
    auto savePath = Gui::GetSavePath();
    if (savePath.isEmpty()) return;
    if (savePath.right(4) != ".png") savePath += ".png";
    setParameters();

    displayProgressBar("Reading JSON...");
    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &NewGeoJsonWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &NewGeoJsonWindow::receiveProgressError, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressReset, this, &NewGeoJsonWindow::receiveProgressReset, Qt::DirectConnection);
    auto img = io.CreateRGB_VectorShapes(parameters);
    Png::SavePng(img, savePath);
    hideProgressBar();
}

void NewGeoJsonWindow::receiveProgressUpdate(uint32_t progress)
{
    ui->progressBar->setValue(progress);
}

void NewGeoJsonWindow::receiveProgressError()
{
    hideProgressBar();
}

void NewGeoJsonWindow::receiveProgressReset(QString desc)
{
    displayProgressBar(desc);
}

void NewGeoJsonWindow::receiveLuaScript(const std::string &script)
{
    parameters.luaScript = script;
    Util::changeSuccessState(ui->label_success, Util::SuccessStateColor::Green);
}

void NewGeoJsonWindow::receivePreviewRequest(const std::string &script)
{
    parameters.luaScript = script;
    previewImage();
}

bool NewGeoJsonWindow::checkInput()
{
    if (!parameters.luaScript.has_value()) {
        Gui::ThrowError("You must configure the Lua script first");
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

void NewGeoJsonWindow::setParameters()
{
    auto startX = ui->doubleSpinBox_startX->value();
    auto startY = ui->doubleSpinBox_startY->value();
    auto endX = ui->doubleSpinBox_endX->value();
    auto endY = ui->doubleSpinBox_endY->value();
    if (startX == 0 && startY == 0 && endX == 0 && endY == 0) {
        parameters.boundaries = std::nullopt;
    }
    else parameters.boundaries = {startX, endX, startY, endY};

    parameters.inputPath = ui->lineEdit_inputPath->text();
    parameters.width = ui->lineEdit_width->value();
    parameters.height = ui->lineEdit_height->value();
}

void NewGeoJsonWindow::previewImage()
{
    if (!checkInput()) return;
    setParameters();
    displayProgressBar("Reading JSON...");
    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &NewGeoJsonWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &NewGeoJsonWindow::receiveProgressError, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressReset, this, &NewGeoJsonWindow::receiveProgressReset, Qt::DirectConnection);
    auto img = io.CreateRGB_VectorShapes(parameters);
    auto task = new PreviewTask<uint8_t>(img);
    QThreadPool::globalInstance()->start(task);
    hideProgressBar();
}






