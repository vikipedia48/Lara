#include "imageconverter.h"
#include "newcsvwindow.h"
#include "previewtask.h"
#include "qtfunctions.h"
#include "ui_newcsvwindow.h"

#include "pngfunctions.h"
#include <QFileInfo>
#include <QThreadPool>

NewCsvWindow::NewCsvWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NewCsvWindow)
{
    ui->setupUi(this);
    luaCodeWindow = nullptr;
    this->setWindowTitle("CSV Converter");
    ui->progressBar->setVisible(false);
    ui->label_progress->setVisible(false);
}

NewCsvWindow::~NewCsvWindow()
{
    delete luaCodeWindow;
    delete ui;
}
#define displayProgressBar(desc) Util::displayProgressBar(ui->progressBar, ui->label_progress, desc);
#define hideProgressBar() Util::hideProgressBar(ui->progressBar, ui->label_progress);

void NewCsvWindow::receivePreviewRequest(const std::string& script)
{
    params.luaScript = script;
    previewImage();
}

void NewCsvWindow::receiveLuaScript(const std::string &script)
{
    params.luaScript = script;
    Util::changeSuccessState(ui->label_success, Util::SuccessStateColor::Green);
}

void NewCsvWindow::on_pushButton_inputPath_clicked()
{
    ui->lineEdit_inputPath->setText(Gui::GetInputPath("Choose csv file","CSV files (*.csv)"));
    if (!checkIfEmpty(ui->lineEdit_inputPath)) {
        auto columns = Util::getAllCsvColumns(ui->lineEdit_inputPath->text().toStdString());
        if (columns.size() < 2) {
            Gui::ThrowError("CSV file is invalid because it has less than two columns");
            ui->lineEdit_inputPath->setText("");
            return;
        }
        for(auto& v : columns) ui->comboBox_xValues->addItem(QString::fromStdString(v));
        for(auto& v : columns) ui->comboBox_yValues->addItem(QString::fromStdString(v));
    }
    ui->pushButton_inputPath->setEnabled(false);
}


void NewCsvWindow::on_pushButton_configure_clicked()
{
    if (luaCodeWindow != nullptr) {
        if (luaCodeWindow->isVisible()) {
            Gui::ThrowError("Configuration window already opened");
            return;
        }
    }
    luaCodeWindow = new LuaCodeWindow(Util::OutputMode::RGB_Points, params.luaScript.value_or(""));
    connect(luaCodeWindow,&LuaCodeWindow::sendCode,this,&NewCsvWindow::receiveLuaScript);
    connect(luaCodeWindow,&LuaCodeWindow::sendPreviewRequest,this,&NewCsvWindow::receivePreviewRequest);
    luaCodeWindow->show();
}


void NewCsvWindow::on_pushButton_preview_clicked()
{
    previewImage();
}


void NewCsvWindow::on_pushButton_reset_clicked()
{
    if (!Gui::GiveQuestion("Are you sure you want to reset your settings?")) {
        return;
    }
    if (luaCodeWindow != nullptr) {
        luaCodeWindow->close();
        luaCodeWindow = nullptr;
    }
    params = NewCsvConvertParams();
    ui->lineEdit_inputPath->clear();
    ui->pushButton_inputPath->setEnabled(true);
    Util::changeSuccessState(ui->label_success, Util::SuccessStateColor::Red);

    ui->lineEdit_width->setValue(1);
    ui->lineEdit_height->setValue(1);
    ui->comboBox_xValues->clear();
    ui->comboBox_yValues->clear();
    ui->doubleSpinBox_startX->setValue(0);
    ui->doubleSpinBox_startY->setValue(0);
    ui->doubleSpinBox_endX->setValue(0);
    ui->doubleSpinBox_endY->setValue(0);
}

void NewCsvWindow::on_pushButton_save_clicked()
{
    exportImage();
}

void NewCsvWindow::receiveProgressUpdate(uint32_t progress)
{
    ui->progressBar->setValue(progress);
}

void NewCsvWindow::receiveProgressError()
{
    hideProgressBar();
}

bool NewCsvWindow::checkIfEmpty(QLineEdit* lineEdit, bool printError)
{
    if (lineEdit->text().isEmpty()) {
        if (printError) Gui::ThrowError("Input empty.");
        return true;
    }
    return false;
}

bool NewCsvWindow::checkInput()
{
    if (checkIfEmpty(ui->lineEdit_inputPath)) return false;
    if (!params.luaScript.has_value()) {
        Gui::ThrowError("You must set the proper configurations first");
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

void NewCsvWindow::setParameters()
{
    auto startX = ui->doubleSpinBox_startX->value();
    auto startY = ui->doubleSpinBox_startY->value();
    auto endX = ui->doubleSpinBox_endX->value();
    auto endY = ui->doubleSpinBox_endY->value();
    if (startX == 0 && startY == 0 && endX == 0 && endY == 0) params.boundaries = std::nullopt;
    else params.boundaries = {startX, endX, startY, endY};

    auto width = ui->lineEdit_width->value();
    auto height = ui->lineEdit_height->value();
    auto indexX = ui->comboBox_xValues->currentIndex();
    auto indexY = ui->comboBox_yValues->currentIndex();

    params.inputPath = ui->lineEdit_inputPath->text();
    params.width = width;
    params.height = height;
    params.coordinateIndexes = {static_cast<uint32_t>(indexX), static_cast<uint32_t>(indexY)};
}

void NewCsvWindow::previewImage()
{
    if (!checkInput()) return;
    setParameters();

    displayProgressBar("Creating the image...");
    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &NewCsvWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &NewCsvWindow::receiveProgressError, Qt::DirectConnection);
    auto buf = io.CreateRGB_Points(params);
    auto img = Png::CreatePngData(buf.get(), {params.width, params.height}, Util::PixelSize::ThirtyTwoBit, true);
    auto displayImageTask = new PreviewTask<uint8_t>(img);
    QThreadPool::globalInstance()->start(displayImageTask);
    hideProgressBar();
}

void NewCsvWindow::exportImage()
{
    if (!checkInput()) return;
    setParameters();
    auto path = Gui::GetSavePath();
    if (path.isEmpty()) return;
    if (path.right(4) != ".png") path.append(".png");
    displayProgressBar("Creating " + QFileInfo(path).fileName() + "...");
    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &NewCsvWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &NewCsvWindow::receiveProgressError, Qt::DirectConnection);
    auto buf = io.CreateRGB_Points(params);
    auto img = Png::CreatePngData(buf.get(), {params.width, params.height}, Util::PixelSize::ThirtyTwoBit, true);
    displayProgressBar("Compressing to PNG...");
    Png::SavePng(img, path);
    hideProgressBar();
}






