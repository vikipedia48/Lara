#include "newgeopackagewindow.h"
#include "qtfunctions.h"
#include "thirdparty/sqlite3/sqlite_modern_cpp.h"
#include "imageconverter.h"
#include "previewtask.h"
#include "ui_newgeopackagewindow.h"
#include "pngfunctions.h"

#include <QThreadPool>

NewGeoPackageWindow::NewGeoPackageWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NewGeoPackageWindow)
{
    ui->setupUi(this);
    luaCodeWindow = nullptr;
    ui->progressBar->setVisible(false);
    ui->label_progress->setVisible(false);
}

NewGeoPackageWindow::~NewGeoPackageWindow()
{
    delete luaCodeWindow;
    delete ui;
}

#define displayProgressBar(desc) Util::displayProgressBar(ui->progressBar, ui->label_progress, desc);
#define hideProgressBar() Util::hideProgressBar(ui->progressBar, ui->label_progress);

void NewGeoPackageWindow::on_pushButton_inputPath_clicked()
{
    ui->lineEdit_inputPath->setText(Gui::GetInputPath("Choose GeoPackage file","GeoPackage files (*.gpkg)"));
    if (!ui->lineEdit_inputPath->text().isEmpty()) {
        if (!loadLayers(ui->lineEdit_inputPath->text())) {
            ui->lineEdit_inputPath->clear();
            return;
        }
        ui->pushButton_inputPath->setEnabled(false);
    }
}

void NewGeoPackageWindow::on_pushButton_configure_clicked()
{
    if (ui->lineEdit_inputPath->text().isEmpty()) {
        Gui::ThrowError("You must select the file first");
        return;
    }
    if (luaCodeWindow != nullptr) {
        if (luaCodeWindow->isVisible()) {
            Gui::ThrowError("LuaCodeWindow already opened.");
            return;
        }
    }
    luaCodeWindow = new LuaCodeWindow(Util::OutputMode::RGB_Gpkg, parameters.luaScript.value_or(""));
    connect(luaCodeWindow,&LuaCodeWindow::sendCode,this,&NewGeoPackageWindow::receiveLuaScript);
    connect(luaCodeWindow,&LuaCodeWindow::sendPreviewRequest,this,&NewGeoPackageWindow::receivePreviewRequest);
    luaCodeWindow->show();
}

void NewGeoPackageWindow::on_pushButton_preview_clicked()
{
    previewImage();
}


void NewGeoPackageWindow::on_pushButton_reset_clicked()
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
    layers.clear();
    for (auto v : layerChecks) v->deleteLater();
    layerChecks.clear();
    parameters = {};
    Util::changeSuccessState(ui->label_success, Util::SuccessStateColor::Red);
}

void NewGeoPackageWindow::receiveLuaScript(const std::string &script)
{
    parameters.luaScript = script;
    Util::changeSuccessState(ui->label_success, Util::SuccessStateColor::Green);
}

void NewGeoPackageWindow::receivePreviewRequest(const std::string &script)
{
    parameters.luaScript = script;
    previewImage();
}

void NewGeoPackageWindow::receiveProgressUpdate(uint32_t progress)
{
    ui->progressBar->setValue(progress);
}

void NewGeoPackageWindow::receiveProgressError()
{
    hideProgressBar();
}

void NewGeoPackageWindow::receiveProgressReset(QString desc)
{
    displayProgressBar(desc);
}

bool NewGeoPackageWindow::loadLayers(const QString &path)
{
    try {
        sqlite::database db(path.toStdString());
        db << "select table_name, data_type, identifier from gpkg_contents;" >> [this](std::string table_name, std::string data_type, std::string identifier) {
            Util::GpkgLayer layer;
            layer.tableName = table_name;
            layer.identifier = identifier;
            layer.dataType = Util::gpkgLayerTypeFromString(data_type);
            layers.push_back(layer);
        };

        for (auto& v : layers) {
            if (v.dataType == Util::GpkgLayerType::Features) {
                auto checkBox = new QCheckBox(QString::fromStdString(v.identifier));
                layerChecks.push_back(checkBox);
                ui->verticalLayout_checkBoxes->addWidget(checkBox);
            }
        }
        if (layerChecks.empty()) throw std::invalid_argument("");
        return true;
    } catch(std::exception& e) {
        Gui::ThrowError("Selected file is not a valid GeoPackage file");
        return false;
    }
}

bool NewGeoPackageWindow::checkInput()
{
    if (std::count_if(layerChecks.begin(), layerChecks.end(), [](QCheckBox* box) { return box->isChecked(); }) == 0) {
        Gui::ThrowError("You must select at least one layer to convert");
        return false;
    }
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

void NewGeoPackageWindow::setParameters()
{
    auto startX = ui->doubleSpinBox_startX->value();
    auto startY = ui->doubleSpinBox_startY->value();
    auto endX = ui->doubleSpinBox_endX->value();
    auto endY = ui->doubleSpinBox_endY->value();
    if (startX == 0 && startY == 0 && endX == 0 && endY == 0) {
        parameters.boundaries = std::nullopt;
    }
    else parameters.boundaries = {startX, endX, startY, endY};

    parameters.selectedLayers = {};
    for (auto i = 0; i < layerChecks.size(); ++i) if (layerChecks[i]->isChecked()) parameters.selectedLayers.push_back(layers[i].tableName);

    parameters.inputPath = ui->lineEdit_inputPath->text();
    parameters.width = ui->lineEdit_width->value();
    parameters.height = ui->lineEdit_height->value();
}

void NewGeoPackageWindow::previewImage()
{
    if (!checkInput()) return;
    setParameters();
    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &NewGeoPackageWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &NewGeoPackageWindow::receiveProgressError, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressReset, this, &NewGeoPackageWindow::receiveProgressReset, Qt::DirectConnection);
    auto img = io.CreateRGB_GeoPackage(parameters);
    auto task = new PreviewTask<uint8_t>(img);
    QThreadPool::globalInstance()->start(task);
    hideProgressBar();
}

void NewGeoPackageWindow::on_pushButton_save_clicked()
{
    if (!checkInput()) return;
    auto savePath = Gui::GetSavePath();
    if (savePath.isEmpty()) return;
    if (savePath.right(4) != ".png") savePath += ".png";
    setParameters();

    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &NewGeoPackageWindow::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &NewGeoPackageWindow::receiveProgressError, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressReset, this, &NewGeoPackageWindow::receiveProgressReset, Qt::DirectConnection);
    auto img = io.CreateRGB_GeoPackage(parameters);
    Png::SavePng(img, savePath);
    hideProgressBar();
}

