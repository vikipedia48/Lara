#include "luacodewindow.h"
#include "qtfunctions.h"
#include "ui_luacodewindow.h"
#include "sol/sol.hpp"

LuaCodeWindow::LuaCodeWindow(Util::OutputMode outputMode, const std::string& script, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LuaCodeWindow), functionType(outputMode)
{
    ui->setupUi(this);
    setWindowTitle("Lua Code Window");
    initFunction();
    if (script.empty()) return;
    try {
        size_t beginIndex, substrSize;
        switch(outputMode) {
            case Util::OutputMode::Grayscale16_Lua:
            beginIndex = G16LuaHeaderSize + 1;
            substrSize = script.size() - G16LuaFooterSize - beginIndex - 1;
            break;
            case Util::OutputMode::RGB_Lua:
            beginIndex = RGBLuaHeaderSize + 1;
            substrSize = script.size() - RGBLuaFooterSize - beginIndex - 1;
            break;
            case Util::OutputMode::RGB_Points:
            beginIndex = CSVLuaHeaderSize + 1;
            substrSize = script.size() - CSVLuaFooterSize - beginIndex - 1;
            break;
            case Util::OutputMode::RGB_Vector:
            beginIndex = GeoJsonLuaHeaderSize + 1;
            substrSize = script.size() - GeoJsonLuaFooterSize - beginIndex - 1;
            break;
            case Util::OutputMode::RGB_Gpkg:
            beginIndex = GpkgLuaHeaderSize + 1;
            substrSize = script.size() - GpkgLuaFooterSize - beginIndex - 1;
            break;
        }
        std::string body = script.substr(beginIndex, substrSize);
        ui->textEdit_code->setText(QString::fromStdString(body));
    } catch(std::exception ex) {
        return;
    }
}

LuaCodeWindow::~LuaCodeWindow()
{
    delete ui;
}


void LuaCodeWindow::on_pushButton_check_clicked()
{
    verifyCode();
}


void LuaCodeWindow::on_pushButton_ok_clicked()
{
    if (!verifyCode()) return;
    emit sendCode(createCode());
    close();
}

void LuaCodeWindow::on_pushButton_cancel_clicked()
{
    if (Gui::GiveQuestion("Are you sure you want to cancel?")) close();
}

void LuaCodeWindow::initFunction()
{
    switch(functionType) {
        case Util::OutputMode::RGB_Lua :
        ui->label_functionHeader->setText(QString::fromStdString(RGBLuaHeader));
        ui->label_functionFooter->setText(QString::fromStdString(RGBLuaFooter));
        break;
        case Util::OutputMode::Grayscale16_Lua :
        ui->label_functionHeader->setText(QString::fromStdString(G16LuaHeader));
        ui->label_functionFooter->setText(QString::fromStdString(G16LuaFooter));
        break;
        case Util::OutputMode::RGB_Points :
        ui->label_functionHeader->setText(QString::fromStdString(CSVLuaHeader));
        ui->label_functionFooter->setText(QString::fromStdString(CSVLuaFooter));
        break;
        case Util::OutputMode::RGB_Vector:
        ui->label_functionHeader->setText(QString::fromStdString(GeoJsonLuaHeader));
        ui->label_functionFooter->setText(QString::fromStdString(GeoJsonLuaFooter));
        break;
        case Util::OutputMode::RGB_Gpkg:
        ui->label_functionHeader->setText(QString::fromStdString(GpkgLuaHeader));
        ui->label_functionFooter->setText(QString::fromStdString(GpkgLuaFooter));
        break;
    }
}

std::string LuaCodeWindow::createCode()
{
    return (ui->label_functionHeader->text() + "\n" + ui->textEdit_code->toPlainText() + "\n" + ui->label_functionFooter->text()).toStdString();
}

bool LuaCodeWindow::verifyCode()
{
    auto code = createCode();
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::table);
    try {
        auto result = lua.script(code);
        if (result.valid()) {
            sol::function fx = lua["set_color"];
            if (!fx.valid()) {
                throw sol::error("invalid function");
            }
            Gui::PrintMessage("OK", "Valid code");
        }
        else {
            throw sol::error("invalid function");
        }
    } catch(sol::error ex) {
        Gui::ThrowError(ex.what());
        return false;
    }
    return true;
}


void LuaCodeWindow::on_pushButton_preview_clicked()
{
    if (!verifyCode()) return;
    emit sendPreviewRequest(createCode());
}

