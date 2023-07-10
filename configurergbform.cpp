#include "configurergbform.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "ui_configurergbform.h"
#include "qtfunctions.h"
#include "imageconverter.h"
#include <QFile>
#include <rapidcsv.h>

ConfigureRGBForm::ConfigureRGBForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConfigureRGBForm)
{
    ui->setupUi(this);
}

ConfigureRGBForm::ConfigureRGBForm(const QString& inputPath, Util::OutputMode mode, QWidget *parent) : QWidget(parent), ui(new Ui::ConfigureRGBForm), mode(mode)
{
    setWindowModality(Qt::WindowModal);
    ui->setupUi(this);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->setWindowTitle("Configure RGB");
    auto title = " ", desc = " ";
    switch(mode) {
        case Util::OutputMode::RGB_UserRanges:
            title = "RGB with user defined ranges";
            desc = "Clicking a cell on the last row will add a new one.";
            ui->checkBox_gradient->setEnabled(true);
            break;
        case Util::OutputMode::RGB_UserValues:
            title = "RGB with user defined values";
            desc = "If you are loading an image with many distinct values, it might take a long time to compute.";
            break;
        case Util::OutputMode::RGB_Points:
            title = "RGB with values from CSV file";
            desc = "The first row's field !SHAPE COLOR! must be set. This is the default color when none of the other conditions are met or when an error occurs.\n"
                   "For further instructions, check the README.";
            ui->checkBox_gradient->setVisible(false);
            ui->tableWidget->clear();
            break;
        case Util::OutputMode::RGB_Vector:
            title = "RGB with values from GeoJson file";
            desc = "The first row must be set with valid values. These are the default values when none of the other conditions are met or when an error occurs.\n"
                   "For further instructions, check the README.";
            ui->checkBox_gradient->setVisible(false);
            ui->tableWidget->clear();
            break;
        default:
            this->close();
            return;
    }
    ui->label_title->setText(title);
    ui->label_desc->setText(desc);
    ui->lineEdit->setText(inputPath);
    ui->lineEdit->setEnabled(false);
    ui->pushButton_preview->setEnabled(false);
    ui->progressBar->setVisible(false);
    ui->label_progress->setVisible(false);
}

ConfigureRGBForm::~ConfigureRGBForm()
{
    delete ui;
}

#define displayProgressBar(desc) Util::displayProgressBar(ui->progressBar, ui->label_progress, desc);
#define hideProgressBar() Util::hideProgressBar(ui->progressBar, ui->label_progress);


void ConfigureRGBForm::on_pushButton_clear_clicked()
{
    if (mode == Util::OutputMode::RGB_UserRanges) {
        ui->tableWidget->item(0,1)->setText("");
        ui->tableWidget->item(1,1)->setText("");
        ui->tableWidget->item(2,0)->setText("");
        ui->tableWidget->item(2,1)->setText("");
        for (auto i = ui->tableWidget->rowCount()-1; i > 2; --i) {
            ui->tableWidget->removeRow(i);
        }
    }
    else if (mode == Util::OutputMode::RGB_UserValues) {
        for (auto i = 0; i < ui->tableWidget->rowCount(); ++i) {
            ui->tableWidget->item(i,1)->setText("");
        }
    }
    else if (mode == Util::OutputMode::RGB_Points || mode == Util::OutputMode::RGB_Vector) {
        for (auto i = ui->tableWidget->rowCount()-1; i > 0; --i) {
            ui->tableWidget->removeRow(i);
        }
        for (auto i = 0; i < ui->tableWidget->columnCount(); ++i) {
                ui->tableWidget->item(0,i)->setText("");
        }
    }
}


CsvConvertParams ConfigureRGBForm::prepareCsvParams(bool &ok)
{
    try {
        CsvConvertParams params;

        if (isRowEmpty(0)) throw std::invalid_argument("First row cannot be empty. It is used for default values.");
        auto rowCount = ui->tableWidget->rowCount();
        auto columnCount = ui->tableWidget->columnCount();
        for (auto row = 0; row < rowCount; ++row) {
            if (isRowEmpty(row)) continue;

            bool ok;
            auto shapeColor = Util::stringToColor(ui->tableWidget->item(row,0)->text(), ok);
            if (!ok) throw std::invalid_argument("Shape color at row " + std::to_string(row) + " is invalid");

            auto centerColorText = ui->tableWidget->item(row,1)->text();
            if (centerColorText.isEmpty()) ui->tableWidget->item(row,1)->setText(ui->tableWidget->item(row,0)->text());

            auto centerColor = Util::stringToColor(ui->tableWidget->item(row,1)->text(), ok);
            if (!ok) throw std::invalid_argument("Center color at row " + std::to_string(row) + " is invalid");

            auto shapeType = Util::csvShapeTypeFromString(ui->tableWidget->item(row,2)->text());
            if (shapeType == Util::CsvShapeType::Error)
                throw std::invalid_argument("Shape type at row " + std::to_string(row) + " is invalid. Valid values are: Square, EmptySquare, Circle and EmptyCircle.");

            auto shapeSize = ui->tableWidget->item(row, 3)->text().toUInt(&ok);
            if (!ok || shapeSize < 1) throw std::invalid_argument("Shape size at row " + std::to_string(row) + " is invalid. It must be a number larger than 0.");

            params.colors.emplace_back(std::pair<color, color>(shapeColor, centerColor));
            params.shapes.emplace_back(std::pair<Util::CsvShapeType, unsigned int>(shapeType, shapeSize));

            if (row == 0) {
                params.columnValues.emplace_back(std::pair<int,QString>());
                continue;
            }

            for (auto col = 4; col < columnCount; ++col) {
                if (!ui->tableWidget->item(row,col)->text().isEmpty()) {
                    params.columnValues.emplace_back(std::pair<int,QString>(col-4, ui->tableWidget->item(row,col)->text()));
                    break;
                }
            }
            if (params.columnValues.size() != params.colors.size()) throw std::invalid_argument("Shape in row " + std::to_string(row) + " not bound to any value");

        }
        ok = true;
        return params;
    } catch(std::invalid_argument e) {
        ok = false;
        Gui::ThrowError(e.what());
        return {};
    }
}

GeoJsonConvertParams ConfigureRGBForm::prepareGeoJsonParams(bool &allOk)
{
    try {
        GeoJsonConvertParams params;
        bool ok;
        auto defaultColor = Util::stringToColor(ui->tableWidget->item(0, 0)->text(), ok);
        if (!ok) throw std::invalid_argument("Default color is invalid");

        params.colors.push_back(defaultColor);
        params.columnValues.emplace_back(std::pair<QString,QString>());

        auto rowCount = ui->tableWidget->rowCount();
        auto columnCount = ui->tableWidget->columnCount();

        for (auto row = 1; row < rowCount; ++row) {
            if (isRowEmpty(row)) continue;
            auto color = Util::stringToColor(ui->tableWidget->item(row, 0)->text(), ok);
            if (!ok) throw std::invalid_argument("Invalid color at row " + std::to_string(row));
            params.colors.push_back(color);
            for (auto col = 1; col < columnCount; ++col) {
                if (!ui->tableWidget->item(row, col)->text().isEmpty()) {
                    params.columnValues.emplace_back(std::pair<QString,QString>(ui->tableWidget->horizontalHeaderItem(col)->text(), ui->tableWidget->item(row,col)->text()));
                    break;
                }
            }
            if (params.colors.size() != params.columnValues.size()) throw std::invalid_argument("Color in row " + std::to_string(row) + " not bound to any value");
        }
        allOk = true;
        return params;
    } catch(std::invalid_argument e) {
        allOk = false;
        Gui::ThrowError(e.what());
        return {};
    }
}



void ConfigureRGBForm::on_pushButton_ok_clicked()
{
    if (mode == Util::OutputMode::RGB_Points) {
        bool ok;
        auto params = prepareCsvParams(ok);
        if (!ok) return;
        emit sendCsvParams(params);
        this->close();
    }
    else if (mode == Util::OutputMode::RGB_Vector) {
        bool ok;
        auto params = prepareGeoJsonParams(ok);
        if (!ok) return;
        emit sendGeoJsonParams(params);
        this->close();
    }
    else {
        auto colorMap = readTable();
        if (colorMap.empty()) return;
        emit sendColorValues(colorMap);
        if (mode == Util::OutputMode::RGB_UserRanges) emit sendGradient(ui->checkBox_gradient->isChecked());
    }
    this->close();
}


void ConfigureRGBForm::on_pushButton_cancel_clicked()
{
    if (Gui::GiveQuestion("Are you sure want to exit?")) this->close();
}

void ConfigureRGBForm::on_pushButton_2_clicked()
{
    auto path = ui->lineEdit->text();
    if (path.isEmpty() || !QFile::exists(path)) {
        Gui::ThrowError("No such file exists.");
        return;
    }

    auto io = ImageConverter();
    connect(&io, &ImageConverter::sendProgress, this, &ConfigureRGBForm::receiveProgressUpdate, Qt::DirectConnection);
    connect(&io, &ImageConverter::sendProgressError, this, &ConfigureRGBForm::receiveProgressError, Qt::DirectConnection);

    if (mode == Util::OutputMode::RGB_UserRanges) {
        double min, max;
        displayProgressBar("Finding min and max values...");
        auto ok = io.GetMinAndMaxValues(path, min, max);
        if (!ok) return;
        addNewRowToTable(QString::number(min, 'g', 17), "0,0,0,255", false);
        addNewRowToTable(QString::number(max, 'g', 17), "255,255,255,255", false);
        addNewRowToTable();
        hideProgressBar();
    }
    else if (mode == Util::OutputMode::RGB_UserValues) {
        displayProgressBar("Finding all distinct values...");
        auto distinctValues = io.GetDistinctValues(path);
        if (distinctValues.empty()) return;
        for (auto value : distinctValues) addNewRowToTable(QString::number(value,'g',17),"0,0,0,255",false);
        hideProgressBar();
    }
    else if (mode == Util::OutputMode::RGB_Points) {
        auto columns = getAllCsvColumns(path.toStdString());
        if (columns.empty()) return;
        if (columns.size() == 1) {
            Gui::ThrowError("CSV file is valid, but only has 1 column, whereas 2 are needed to specify x and y");
            return;
        }
        QStringList headers = {"!SHAPE COLOR!", "!CENTER COLOR!", "!SHAPE TYPE!", "!SHAPE SIZE!"};
        for (auto& v : columns) {
            headers.push_back(QString::fromStdString(v));
        }
        ui->tableWidget->setColumnCount(headers.count());
        ui->tableWidget->setHorizontalHeaderLabels(headers);
        addNewRowToTable(Util::OutputMode::RGB_Points);
    }
    else if (mode == Util::OutputMode::RGB_Vector) {
        QJsonObject properties;
        if (!getPropertiesFromGeoJson(path, properties)) return;
        QStringList headers = {"!COLOR!", "!GEOMETRY TYPE!"};
        for (auto& v : properties.keys()) {
            if (properties[v].isObject() || properties[v].isArray()) continue;
            headers.push_back(v);
        }
        ui->tableWidget->setColumnCount(headers.count());
        ui->tableWidget->setHorizontalHeaderLabels(headers);
        addNewRowToTable(Util::OutputMode::RGB_Vector);
    }
    ui->pushButton_clear->setEnabled(true);
    ui->pushButton_preview->setEnabled(true);
    ui->pushButton_ok->setEnabled(true);
}

std::pair<double, color> ConfigureRGBForm::getRowValues(int row, bool &ok)
{
    bool _ok;
    auto value = ui->tableWidget->item(row,0)->text().toDouble(&_ok);
    if (!_ok) {
        Gui::ThrowError("Invalid value at row " + QString::number(row));
        ok = false;
        return {};
    }
    auto colorValue = Util::stringToColor(ui->tableWidget->item(row,1)->text(), _ok);
    if (!_ok) {
        Gui::ThrowError("Invalid color at row " + QString::number(row));
        ok = false;
        return {};
    }
    ok = true;
    return {value,colorValue};
}

void ConfigureRGBForm::addNewRowToTable(QString firstValue, QString secondValue, bool isFirstColumnEditable)
{
    auto row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    auto item0 = new QTableWidgetItem(firstValue);
    auto item1 = new QTableWidgetItem(secondValue);
    if (!isFirstColumnEditable) item0->setFlags(item0->flags() ^ Qt::ItemIsEditable);
    ui->tableWidget->setItem(row,0,item0);
    ui->tableWidget->setItem(row,1,item1);
}

void ConfigureRGBForm::addNewRowToTable(Util::OutputMode pointsMode)
{
    if (pointsMode == Util::OutputMode::RGB_UserRanges || pointsMode == Util::OutputMode::RGB_UserValues) {
        addNewRowToTable();
    }
    else {
        auto row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
        for (auto i = 0; i < ui->tableWidget->columnCount(); ++i) ui->tableWidget->setItem(row, i, new QTableWidgetItem());
    }
}


std::map<double, color> ConfigureRGBForm::readTable()
{
    std::map<double,color> colorMap;
    for (auto i = 0; i < ui->tableWidget->rowCount(); ++i) {
        if (isRowEmpty(i)) continue;
        bool ok;
        colorMap.emplace(getRowValues(i,ok));
        if (!ok) return {};
    }
    return colorMap;
}

bool ConfigureRGBForm::isRowEmpty(int row)
{
    for (auto i = 0; i < ui->tableWidget->columnCount(); ++i) {
        if (!ui->tableWidget->item(row,i)->text().isEmpty()) return false;
    }
    return true;
}

void ConfigureRGBForm::on_pushButton_preview_clicked()
{
    if (mode == Util::OutputMode::RGB_Points) {
        bool ok;
        auto params = prepareCsvParams(ok);
        if (!ok) return;
        emit sendPreviewRequest(params);
    }
    else if (mode == Util::OutputMode::RGB_Vector) {
        bool ok;
        auto params = prepareGeoJsonParams(ok);
        if (!ok) return;
        emit sendPreviewRequest(params);
    }
    else {
        auto colorMap = readTable();
        if (mode == Util::OutputMode::RGB_UserRanges) emit sendPreviewRequest(colorMap, ui->checkBox_gradient->isChecked());
        else if (mode == Util::OutputMode::RGB_UserValues) emit sendPreviewRequest(colorMap);
    }
}


void ConfigureRGBForm::on_tableWidget_cellClicked(int row, int column)
{
    if ((mode == Util::OutputMode::RGB_UserRanges || mode == Util::OutputMode::RGB_Vector || mode == Util::OutputMode::RGB_Points) && ui->tableWidget->rowCount() == row+1) addNewRowToTable(mode);
}

void ConfigureRGBForm::receiveProgressUpdate(uint32_t progress)
{
    ui->progressBar->setValue(progress);
}

void ConfigureRGBForm::receiveProgressError()
{
    hideProgressBar();
}


bool ConfigureRGBForm::getPropertiesFromGeoJson(const QString &path, QJsonObject& outputProperties)
{
    try {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) throw std::invalid_argument("cannot open file " + path.toStdString());
        auto bytes = file.readAll();
        QJsonParseError error;
        auto document = QJsonDocument::fromJson(bytes, &error);
        if (error.error != QJsonParseError::NoError) throw std::invalid_argument(error.errorString().toStdString());
        if (!document.isObject()) throw std::invalid_argument("document isn't an object");
        auto jsonObj = document.object();
        auto features = jsonObj["features"];
        if (features == QJsonValue::Undefined || !features.isArray()) throw std::invalid_argument("invalid ""features"" object");
        auto firstFeature = features.toArray()[0];
        if (firstFeature == QJsonValue::Undefined || !firstFeature.isObject()) throw std::invalid_argument("first object in ""features"" is invalid");
        auto properties = firstFeature.toObject()["properties"];
        if (properties == QJsonValue::Undefined || !properties.isObject()) throw std::invalid_argument("first object in ""features"" has invalid ""properties""");
        outputProperties = properties.toObject();
    } catch (std::invalid_argument e) {
        Gui::ThrowError("Error reading GeoJson: " + QString(e.what()));
        return false;
    }
    return true;
}

std::vector<std::string> ConfigureRGBForm::getAllCsvColumns(const std::string& path)
{
    auto rv = std::vector<std::string>();
    rapidcsv::Document doc(path);
    rv = doc.GetColumnNames();
    if (rv.empty()) Gui::ThrowError("Invalid csv file");
    return rv;
}

