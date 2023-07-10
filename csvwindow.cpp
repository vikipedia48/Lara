#include "csvwindow.h"
#include "qtfunctions.h"
#include "ui_csvwindow.h"
#include "pngfunctions.h"
#include "previewtask.h"
#include "imageconverter.h"

#include <QDir>
#include <QThreadPool>

#define emptyInput -1
#define invalidInput -2

CSVWindow::CSVWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CSVWindow)
{
    ui->setupUi(this);
    configWindow = nullptr;
    this->setWindowTitle("CSV Converter");
    ui->progressBar->setVisible(false);
    ui->label_progress->setVisible(false);
}

CSVWindow::~CSVWindow()
{
    delete ui;
    delete configWindow;
}

#define displayProgressBar(desc) Util::displayProgressBar(ui->progressBar, ui->label_progress, desc);
#define hideProgressBar() Util::hideProgressBar(ui->progressBar, ui->label_progress);

void CSVWindow::receiveCsvParameters(const CsvConvertParams &params)
{
    this->params = params;
    Util::changeSuccessState(ui->label_success, Util::SuccessStateColor::Green);
}

void CSVWindow::receivePreviewRequest(const CsvConvertParams &params)
{
    this->params = params;
    previewImage();
}

void CSVWindow::on_pushButton_inputPath_clicked()
{
    ui->lineEdit_inputPath->setText(Gui::GetInputPath("Choose csv file","CSV files (*.csv)"));
    ui->pushButton_inputPath->setEnabled(false);
}


void CSVWindow::on_pushButton_configure_clicked()
{
    if (configWindow != nullptr) {
        if (configWindow->isVisible()) {
            Gui::ThrowError("Configuration window already opened");
            return;
        }
    }
    this->configWindow = new ConfigureRGBForm(ui->lineEdit_inputPath->text(), Util::OutputMode::RGB_Points);
    connect(configWindow,SIGNAL(sendCsvParams(const CsvConvertParams&)),this,SLOT(receiveCsvParameters(const CsvConvertParams&)));
    connect(configWindow,SIGNAL(sendPreviewRequest(const CsvConvertParams&)),this,SLOT(receivePreviewRequest(const CsvConvertParams&)));
    configWindow->show();
}


void CSVWindow::on_pushButton_preview_clicked()
{
    previewImage();
}


void CSVWindow::on_pushButton_reset_clicked()
{
    if (!Gui::GiveQuestion("Are you sure you want to reset your settings?")) {
        return;
    }
    params.reset();
    ui->lineEdit_inputPath->clear();
    ui->pushButton_inputPath->setEnabled(true);
    Util::changeSuccessState(ui->label_success, Util::SuccessStateColor::Red);

    ui->lineEdit_width->setValue(1);
    ui->lineEdit_height->setValue(1);
    ui->lineEdit_indexX->setValue(0);
    ui->lineEdit_indexY->setValue(0);
    ui->doubleSpinBox_startX->setValue(0);
    ui->doubleSpinBox_startY->setValue(0);
    ui->doubleSpinBox_endX->setValue(0);
    ui->doubleSpinBox_endY->setValue(0);
}


void CSVWindow::on_pushButton_save_clicked()
{
    exportImage();
}

void CSVWindow::receiveProgressUpdate(uint32_t progress)
{
    ui->progressBar->setValue(progress);
}

void CSVWindow::receiveProgressError()
{
    hideProgressBar();
}

bool CSVWindow::checkIfEmpty(QLineEdit* lineEdit, bool printError)
{
    if (lineEdit->text().isEmpty()) {
        if (printError) Gui::ThrowError("Input empty.");
        return true;
    }
    return false;
}

bool CSVWindow::checkInput()
{
    if (checkIfEmpty(ui->lineEdit_inputPath)) return false;
    if (!params.has_value()) {
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



void CSVWindow::setParameters()
{
    auto startX = ui->doubleSpinBox_startX->value();
    auto startY = ui->doubleSpinBox_startY->value();
    auto endX = ui->doubleSpinBox_endX->value();
    auto endY = ui->doubleSpinBox_endY->value();
    if (startX == 0 && startY == 0 && endX == 0 && endY == 0) params.value().boundaries = std::nullopt;
    else params.value().boundaries = {startX, endX, startY, endY};

    auto width = ui->lineEdit_width->value();
    auto height = ui->lineEdit_height->value();
    auto indexX = ui->lineEdit_indexX->value();
    auto indexY = ui->lineEdit_indexY->value();

    params.value().inputPath = ui->lineEdit_inputPath->text();
    params.value().width = width;
    params.value().height = height;
    params.value().coordinateIndexes = {static_cast<unsigned int>(indexX), static_cast<unsigned int>(indexY)};
}

void CSVWindow::previewImage()
{
    if (!checkInput()) return;
    setParameters();

    displayProgressBar("Creating the image...");
    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &CSVWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &CSVWindow::receiveProgressError, Qt::DirectConnection);
    auto buf = io.CreateRGB_Points(params.value());
    auto img = Png::CreatePngData(buf.get(), {params.value().width, params.value().height}, Util::PixelSize::ThirtyTwoBit, true);
    auto displayImageTask = new PreviewTask<uint8_t>(img);
    QThreadPool::globalInstance()->start(displayImageTask);
    hideProgressBar();
}


void CSVWindow::exportImage()
{
    if (!checkInput()) return;
    setParameters();
    auto path = Gui::GetSavePath();
    if (path.isEmpty()) return;
    if (path.right(4) != ".png") path.append(".png");
    displayProgressBar("Creating " + QFileInfo(path).fileName() + "...");
    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &CSVWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &CSVWindow::receiveProgressError, Qt::DirectConnection);
    auto buf = io.CreateRGB_Points(params.value());
    auto img = Png::CreatePngData(buf.get(), {params.value().width, params.value().height}, Util::PixelSize::ThirtyTwoBit, true);
    Png::SavePng(img, path);
    hideProgressBar();
}








