#include "geotiffwindow.h"
#include "qtfunctions.h"
#include "ui_geotiffwindow.h"
#include "tifffunctions.h"
#include "pngfunctions.h"
#include "previewtask.h"
#include "imageconverter.h"

#include <QDir>
#include <QThreadPool>

GeotiffWindow::GeotiffWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeotiffWindow)
{
    ui->setupUi(this);
    this->configWindow = nullptr;
    this->setWindowTitle("GeoTiff Converter");
    ui->progressBar->setVisible(false);
    ui->label_progress->setVisible(false);
}

GeotiffWindow::~GeotiffWindow()
{
    configWindow->close();
    delete ui;
}

#define displayProgressBar(desc) Util::displayProgressBar(ui->progressBar, ui->label_progress, desc);
#define hideProgressBar() Util::hideProgressBar(ui->progressBar, ui->label_progress);

void GeotiffWindow::on_pushButton_inputFile_clicked()
{
    ui->lineEdit_inputFile->setText(Gui::GetInputPath("Choose tiff file","TIFF files (*.tiff *tif)"));

    auto widthAndHeight = Tiff::GetWidthAndHeight(ui->lineEdit_inputFile->text());
    if (widthAndHeight.first == 0 || widthAndHeight.second == 0) {
        Gui::ThrowError("Selected file is not a valid TIFF image");
        ui->lineEdit_inputFile->clear();
        return;
    }
    ui->pushButton_inputFile->setEnabled(false);
    ui->lineEdit_cropStartX->setValue(0);
    ui->lineEdit_cropStartY->setValue(0);
    ui->lineEdit_cropEndX->setValue(widthAndHeight.first-1);
    ui->lineEdit_cropEndY->setValue(widthAndHeight.second-1);
}


void GeotiffWindow::on_comboBox_outputMode_activated(int index)
{
    if (configWindow != nullptr) {
        if (configWindow->isVisible() && configWindow->mode != getOutputModeSelected()) {
            Gui::ThrowError("Output mode changed. Closing ConfigureRGB window.");
            configWindow->close();
        }
    }
    switch (getOutputModeSelected()) {
        case Util::OutputMode::No:
            Util::changeSuccessState(ui->label_outputModeSuccess, Util::SuccessStateColor::Red);
            break;
        case Util::OutputMode::Grayscale16_TrueValue:
            Util::changeSuccessState(ui->label_outputModeSuccess, !parameters.offset.has_value() ? Util::SuccessStateColor::Yellow : Util::SuccessStateColor::Green);
            break;
        case Util::OutputMode::Grayscale16_MinToMax: case Util::OutputMode::RGB_Formula:
            Util::changeSuccessState(ui->label_outputModeSuccess, Util::SuccessStateColor::Green);
            break;
        case Util::OutputMode::RGB_UserRanges: case Util::OutputMode::RGB_UserValues:
            Util::changeSuccessState(ui->label_outputModeSuccess, !parameters.colorValues.has_value() ? Util::SuccessStateColor::Red : Util::SuccessStateColor::Green);
            break;
        default: break; // unreachable
    }
}


void GeotiffWindow::on_pushButton_outputModeConfigure_clicked()
{
    auto index = getOutputModeSelected();
    switch (index) {
        case Util::OutputMode::No:
            Gui::ThrowError("Please select output mode");
            break;
        case Util::OutputMode::Grayscale16_TrueValue:
            parameters.offset = Gui::GetNumberValueFromInputDialog("Input offset", "Please input the optional offset for values.");
            if (parameters.offset == 0) {
                parameters.offset.reset();
                Util::changeSuccessState(ui->label_outputModeSuccess, Util::SuccessStateColor::Yellow);
            }
            else Util::changeSuccessState(ui->label_outputModeSuccess, Util::SuccessStateColor::Green);
            break;
        case Util::OutputMode::Grayscale16_MinToMax: case Util::OutputMode::RGB_Formula:
            Gui::PrintMessage("No settings","There are no configurations for this mode");
            break;
        case Util::OutputMode::RGB_UserRanges: case Util::OutputMode::RGB_UserValues:
            if (configWindow != nullptr) {
                if (configWindow->isVisible()) {
                    Gui::ThrowError("ConfigureRGB already opened.");
                    return;
                }
            }
            if (parameters.colorValues.has_value()) this->configWindow = new ConfigureRGBForm(ui->lineEdit_inputFile->text(), getOutputModeSelected(), parameters);
            else this->configWindow = new ConfigureRGBForm(ui->lineEdit_inputFile->text(), getOutputModeSelected());
            connect(configWindow,SIGNAL(sendColorValues(const std::map<double,color>&)),this,SLOT(receiveColorValues(const std::map<double,color>&)));
            connect(configWindow,SIGNAL(sendGradient(bool)),this,SLOT(receiveGradient(bool)));
            connect(configWindow,SIGNAL(sendPreviewRequest(const std::map<double,color>&, bool)),this,SLOT(receivePreviewRequest(const std::map<double,color>&, bool)));
            connect(configWindow,SIGNAL(sendPreviewRequest(const std::map<double,color>&)),this,SLOT(receivePreviewRequest(const std::map<double,color>&)));
            configWindow->show();
            break;
        default: break; /* unreachable */
    }
}


void GeotiffWindow::on_comboBox_splitIntoTiles_activated(int index)
{
    if (index==0) {
        ui->comboBox_scaleImage->setEnabled(true);
    }
    else {
        ui->comboBox_scaleImage->setEnabled(false);
    }
}

void GeotiffWindow::on_comboBox_scaleImage_activated(int index)
{
    if (index==0) {
        ui->comboBox_splitIntoTiles->setEnabled(true);
    }
    else {
        ui->comboBox_splitIntoTiles->setEnabled(false);
    }
}

void GeotiffWindow::on_pushButton_reset_clicked()
{
    if (!Gui::GiveQuestion("Are you sure you want to reset your settings?")) {
        return;
    }
    if (configWindow != nullptr) {
        configWindow->close();
        configWindow = nullptr;
    }
    parameters.colorValues.reset();
    parameters.gradient.reset();
    parameters.offset.reset();
    ui->lineEdit_inputFile->clear();
    ui->pushButton_inputFile->setEnabled(true);
    ui->comboBox_outputMode->setCurrentIndex(0);
    Util::changeSuccessState(ui->label_outputModeSuccess, Util::SuccessStateColor::Red);
    ui->lineEdit_cropStartX->setValue(0);
    ui->lineEdit_cropStartY->setValue(0);
    ui->lineEdit_cropEndX->setValue(0);
    ui->lineEdit_cropEndY->setValue(0);
    ui->comboBox_splitIntoTiles->setCurrentIndex(0);
    ui->comboBox_splitIntoTiles->setEnabled(true);
    ui->lineEdit_tilesX->setValue(1);
    ui->lineEdit_tilesY->setValue(1);
    ui->comboBox_scaleImage->setCurrentIndex(0);
    ui->comboBox_splitIntoTiles->setEnabled(true);
    ui->spinBox_scale->setValue(1);
    ui->progressBar->setVisible(false);
}


void GeotiffWindow::on_pushButton_save_clicked()
{
    exportImage();
}


void GeotiffWindow::on_pushButton_preview_clicked()
{
    previewImage();
}

void GeotiffWindow::receiveColorValues(const std::map<double, color> &colors)
{
    if (colors.empty()) {
        parameters.colorValues.reset();
        return;
    }
    parameters.colorValues = colors;
    Util::changeSuccessState(ui->label_outputModeSuccess, Util::SuccessStateColor::Green);
}

void GeotiffWindow::receiveGradient(bool yes)
{
    parameters.gradient = yes;
}

void GeotiffWindow::getTileSize(int &tileSizeX, int &tileSizeY, std::pair<int,int> widthAndHeight)
{
    if (getTileModeSelected() == Util::TileMode::No) return;
    auto t1 = ui->lineEdit_tilesX->value();
    auto t2 = ui->lineEdit_tilesY->value();
    if (getTileModeSelected() == Util::TileMode::DefineBySize) {
        tileSizeX = t1;
        tileSizeY = t2;
    }
    else {
        double c1 = (double)widthAndHeight.first/t1, c2 = (double)widthAndHeight.second/t2;
        tileSizeX = std::ceil(c1);
        tileSizeY = std::ceil(c2);
    }
}

Util::TileMode GeotiffWindow::getTileModeSelected()
{
    return static_cast<Util::TileMode>(ui->comboBox_splitIntoTiles->currentIndex());
}

Util::OutputMode GeotiffWindow::getOutputModeSelected()
{
    return static_cast<Util::OutputMode>(ui->comboBox_outputMode->currentIndex());
}

Util::ScaleMode GeotiffWindow::getScaleModeSelected()
{
    return static_cast<Util::ScaleMode>(ui->comboBox_scaleImage->currentIndex());
}

void GeotiffWindow::receivePreviewRequest(const std::map<double, color> &colorMap, bool gradient)
{
    previewImage(colorMap, gradient);
}

void GeotiffWindow::receivePreviewRequest(const std::map<double, color> &colorMap)
{
    previewImage(colorMap);
}

void GeotiffWindow::receiveProgressUpdate(uint32_t progress)
{
    ui->progressBar->setValue(progress);
}

void GeotiffWindow::receiveProgressError()
{
    hideProgressBar();
}

bool GeotiffWindow::checkInput()
{
    try {
        auto inputPath = ui->lineEdit_inputFile->text();
        if (inputPath.isEmpty()) throw std::invalid_argument("You must select the file's path");
        switch(getOutputModeSelected()) {
            case Util::OutputMode::No: throw std::invalid_argument("You must select the output mode"); break;
            case Util::OutputMode::RGB_UserRanges: case Util::OutputMode::RGB_UserValues:
                if (!parameters.colorValues.has_value()) throw std::invalid_argument("Conversion settings aren't configured"); break;
            default: break;
        }
        auto widthAndHeight = Tiff::GetWidthAndHeight(inputPath);
        if (widthAndHeight.first == 0 || widthAndHeight.second == 0) throw std::invalid_argument("Invalid tiff file");
        std::uint32_t startX = ui->lineEdit_cropStartX->value();
        std::uint32_t startY = ui->lineEdit_cropStartY->value();
        std::uint32_t endX = ui->lineEdit_cropEndX->value();
        std::uint32_t endY = ui->lineEdit_cropEndY->value();
        if (startX >= endX || startY >= endY) throw std::invalid_argument("Invalid cropping");
        return true;
    } catch(std::invalid_argument e) {
        Gui::ThrowError(e.what());
        return false;
    }
}

void GeotiffWindow::setParameters() {
    auto outputMode = getOutputModeSelected();
    std::uint32_t startX = ui->lineEdit_cropStartX->value();
    std::uint32_t startY = ui->lineEdit_cropStartY->value();
    std::uint32_t endX = ui->lineEdit_cropEndX->value();
    std::uint32_t endY = ui->lineEdit_cropEndY->value();

    parameters.inputPath = ui->lineEdit_inputFile->text();
    parameters.outputMode = outputMode;
    parameters.startX = startX;
    parameters.startY = startY;
    parameters.endX = endX;
    parameters.endY = endY;
    parameters.scaleMode = getScaleModeSelected();
    parameters.scale = ui->spinBox_scale->value();
    if (!parameters.gradient.has_value()) parameters.gradient = false;
    if (!parameters.offset.has_value()) parameters.offset = 0;
}


void GeotiffWindow::previewImage(const std::map<double, color> &colorMap, bool gradient)
{
    this->parameters.gradient = gradient;
    this->parameters.colorValues = colorMap;
    previewImage();
}

void GeotiffWindow::previewImage(const std::map<double, color> &colorMap)
{
    this->parameters.colorValues = colorMap;
    previewImage();
}

void GeotiffWindow::previewImage() {
    if (!checkInput()) return;
    setParameters();
    auto params = parameters;
    auto widthAndHeight = std::pair<unsigned int, unsigned int> {params.endX-params.startX+1, params.endY-params.startY+1};

    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &GeotiffWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &GeotiffWindow::receiveProgressError, Qt::DirectConnection);

    if (params.scaleMode != Util::ScaleMode::No) {
        displayProgressBar("Reading raw image values...");
        auto rawValues = io.GetRawImageValues(params.inputPath, params.startX, params.endX, params.startY, params.endY);
        displayProgressBar("Creating the image...");
        if (params.outputMode == Util::OutputMode::RGB_UserValues ||
            params.outputMode == Util::OutputMode::RGB_UserRanges ||
            params.outputMode == Util::OutputMode::RGB_Formula) {
            auto buf = io.CreateImageData_RGB(rawValues.get(), params, widthAndHeight);
            auto img = Png::CreatePngData(buf.get(), widthAndHeight, Util::PixelSize::ThirtyTwoBit);
            auto task = new PreviewTask<uint8_t>(img);
            QThreadPool::globalInstance()->start(task);
        }

        else {
            if (params.outputMode == Util::OutputMode::Grayscale16_MinToMax) {
                params.minAndMax = std::pair<double,double>{};
                params.minAndMax.value().first = *std::min_element(rawValues.get(), rawValues.get()+(widthAndHeight.first*widthAndHeight.second));
                params.minAndMax.value().second = *std::max_element(rawValues.get(), rawValues.get()+(widthAndHeight.first*widthAndHeight.second));
            }
            auto buf = io.CreateImageData_G16(rawValues.get(), params, widthAndHeight);
            auto img = Png::CreatePngData(buf.get(), widthAndHeight, Util::PixelSize::SixteenBit);
            auto task = new PreviewTask<uint16_t>(img);
            QThreadPool::globalInstance()->start(task);
        }
        hideProgressBar();
        return;
    }

    displayProgressBar("Creating the image...");

    auto startX = params.startX;
    auto startY = params.startY;
    auto endX = params.endX;
    auto endY = params.endY;

    switch (params.outputMode) {
        case Util::OutputMode::Grayscale16_MinToMax:
            {
                displayProgressBar("Finding min and max values...");
                params.minAndMax = std::pair<double,double>{};
                auto ok = io.GetMinAndMaxValues(params.inputPath, params.minAndMax.value().first, params.minAndMax.value().second, startX, endX, startY, endY);
                if (!ok) return;
                displayProgressBar("Creating the image...");
                auto buf = io.CreateG16_MinToMax(params.inputPath, params.minAndMax.value(), startX, startY, endX, endY);
                auto img = Png::CreatePngData(buf.get(), widthAndHeight, Util::PixelSize::SixteenBit);
                auto task = new PreviewTask<uint16_t>(img);
                QThreadPool::globalInstance()->start(task);
            }
            break;
        case Util::OutputMode::Grayscale16_TrueValue:
            {
                auto buf = io.CreateG16_TrueValue(params.inputPath, params.offset.value(), startX, startY, endX, endY);
                auto img = Png::CreatePngData(buf.get(), widthAndHeight, Util::PixelSize::SixteenBit);
                auto task = new PreviewTask<uint16_t>(img);
                QThreadPool::globalInstance()->start(task);
            }
            break;
        case Util::OutputMode::RGB_UserValues:
            {
                auto buf = io.CreateRGB_UserValues(params.inputPath, params.colorValues.value(), startX, startY, endX, endY);
                auto img = Png::CreatePngData(buf.get(), widthAndHeight, Util::PixelSize::ThirtyTwoBit);
                auto task = new PreviewTask<uint8_t>(img);
                QThreadPool::globalInstance()->start(task);
            }
        break;
        case Util::OutputMode::RGB_UserRanges:
            {
                auto buf = io.CreateRGB_UserRanges(params.inputPath, params.colorValues.value(), params.gradient.value(), startX, startY, endX, endY);
                auto img = Png::CreatePngData(buf.get(), widthAndHeight, Util::PixelSize::ThirtyTwoBit);
                auto task = new PreviewTask<uint8_t>(img);
                QThreadPool::globalInstance()->start(task);
            }
            break;
        case Util::OutputMode::RGB_Formula:
            {
                auto buf = io.CreateRGB_Formula(params.inputPath, startX, startY, endX, endY);
                auto img = Png::CreatePngData(buf.get(), widthAndHeight, Util::PixelSize::ThirtyTwoBit);
                auto task = new PreviewTask<uint8_t>(img);
                QThreadPool::globalInstance()->start(task);
            }
            break;
        default:
            throw std::invalid_argument("unreachable code");
    }
    hideProgressBar();
}

void GeotiffWindow::exportImage() {
    if (!checkInput()) return;
    setParameters();

    auto path = Gui::GetSavePath();
    if (path.isEmpty()) return;
    auto outputPath = QFileInfo(path).absolutePath()+QDir::separator()+QFileInfo(path).completeBaseName();
    if (path.right(4) != ".png") path.append(".png");

    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &GeotiffWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &GeotiffWindow::receiveProgressError, Qt::DirectConnection);

    auto absoluteStartX = parameters.startX;
    auto absoluteStartY = parameters.startY;
    auto absoluteEndX = parameters.endX;
    auto absoluteEndY = parameters.endY;
    auto absoluteWidthAndHeight = std::pair<unsigned int, unsigned int>(absoluteEndX-absoluteStartX+1, absoluteEndY-absoluteStartY+1);
    auto params = parameters;

    if (params.scaleMode != Util::ScaleMode::No) {
        displayProgressBar("Reading raw image values...");
        auto rawValues = io.GetRawImageValues(params.inputPath, params.startX, params.endX, params.startY, params.endY);
        displayProgressBar("Creating the image...");
        if (params.outputMode == Util::OutputMode::RGB_UserValues ||
            params.outputMode == Util::OutputMode::RGB_UserRanges ||
            params.outputMode == Util::OutputMode::RGB_Formula) {
            auto buf = io.CreateImageData_RGB(rawValues.get(), params, absoluteWidthAndHeight);
            auto img = Png::CreatePngData(buf.get(), absoluteWidthAndHeight, Util::PixelSize::ThirtyTwoBit);
            Png::SavePng(img, path);
        }

        else {
            if (params.outputMode == Util::OutputMode::Grayscale16_MinToMax) {
                params.minAndMax = std::pair<double,double>{};
                params.minAndMax.value().first = *std::min_element(rawValues.get(), rawValues.get()+(absoluteWidthAndHeight.first*absoluteWidthAndHeight.second));
                params.minAndMax.value().second = *std::max_element(rawValues.get(), rawValues.get()+(absoluteWidthAndHeight.first*absoluteWidthAndHeight.second));
            }
            auto buf = io.CreateImageData_G16(rawValues.get(), params, absoluteWidthAndHeight);
            auto img = Png::CreatePngData(buf.get(), absoluteWidthAndHeight, Util::PixelSize::SixteenBit);
            Png::SavePng(img, path);
        }
        hideProgressBar();
        return;
    }

    int tileSize_x = absoluteWidthAndHeight.first, tileSize_y = absoluteWidthAndHeight.second;
    getTileSize(tileSize_x, tileSize_y, absoluteWidthAndHeight);

    for (auto startX = absoluteStartX; startX < absoluteEndX; startX+=tileSize_x) {
        for (auto startY = absoluteStartY; startY < absoluteEndY; startY+=tileSize_y) {
            if (getTileModeSelected() != Util::TileMode::No) path = outputPath+"_"+QString::number((startX-absoluteStartX)/tileSize_x)+"_"+QString::number((startY-absoluteStartY)/tileSize_y)+".png";
            displayProgressBar("Creating " + QFileInfo(path).fileName() + "...");
            auto endX = std::min((unsigned int)startX+tileSize_x-1,absoluteEndX);
            auto endY = std::min((unsigned int)startY+tileSize_y-1,absoluteEndY);
            auto tileWidthAndHeight = std::pair<unsigned int, unsigned int> {endX-startX+1,endY-startY+1};

            switch (params.outputMode) {
                case Util::OutputMode::Grayscale16_MinToMax:
                    {
                        parameters.minAndMax = std::pair<double,double>{};
                        displayProgressBar("Finding min and max values...");
                        auto ok = io.GetMinAndMaxValues(parameters.inputPath, parameters.minAndMax.value().first, parameters.minAndMax.value().second, startX, endX, startY, endY);
                        if (!ok) return;
                        displayProgressBar("Creating " + QFileInfo(path).fileName() + "...");
                        auto buf = io.CreateG16_MinToMax(parameters.inputPath, parameters.minAndMax.value(), startX, startY, endX, endY);
                        auto img = Png::CreatePngData(buf.get(), tileWidthAndHeight, Util::PixelSize::SixteenBit);
                        Png::SavePng(img, path);
                    }
                    break;
                case Util::OutputMode::Grayscale16_TrueValue:
                    {
                        auto buf = io.CreateG16_TrueValue(parameters.inputPath, parameters.offset.value(), startX, startY, endX, endY);
                        auto img = Png::CreatePngData(buf.get(), tileWidthAndHeight, Util::PixelSize::SixteenBit);
                        Png::SavePng(img, path);
                    }
                    break;
                case Util::OutputMode::RGB_UserValues:
                    {
                        auto buf = io.CreateRGB_UserValues(parameters.inputPath, parameters.colorValues.value(), startX, startY, endX, endY);
                        auto img = Png::CreatePngData(buf.get(), tileWidthAndHeight, Util::PixelSize::ThirtyTwoBit);
                        Png::SavePng(img, path);
                    }
                break;
                case Util::OutputMode::RGB_UserRanges:
                    {
                        auto buf = io.CreateRGB_UserRanges(parameters.inputPath, parameters.colorValues.value(), parameters.gradient.value(), startX, startY, endX, endY);
                        auto img = Png::CreatePngData(buf.get(), tileWidthAndHeight, Util::PixelSize::ThirtyTwoBit);
                        Png::SavePng(img, path);
                    }
                    break;
                case Util::OutputMode::RGB_Formula:
                    {
                        auto buf = io.CreateRGB_Formula(parameters.inputPath, startX, startY, endX, endY);
                        auto img = Png::CreatePngData(buf.get(), tileWidthAndHeight, Util::PixelSize::ThirtyTwoBit);
                        Png::SavePng(img, path);
                    }
                    break;
                default:
                    throw std::invalid_argument("unreachable code");
            }
            hideProgressBar();
        }
    }

}


