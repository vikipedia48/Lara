#ifndef LUACODEWINDOW_H
#define LUACODEWINDOW_H

#include "commonfunctions.h"
#include <QWidget>

namespace Ui {
class LuaCodeWindow;
}

class LuaCodeWindow : public QWidget
{
    Q_OBJECT

public:
    Util::OutputMode functionType;
    explicit LuaCodeWindow(Util::OutputMode outputMode, const std::string& script, QWidget* parent = nullptr);
    ~LuaCodeWindow();

signals:
    void sendCode(const std::string& code);
    void sendPreviewRequest(const std::string& code);

private slots:

    void on_pushButton_check_clicked();

    void on_pushButton_ok_clicked();

    void on_pushButton_cancel_clicked();

    void on_pushButton_preview_clicked();

private:
    Ui::LuaCodeWindow *ui;

    void initFunction();
    std::string createCode();
    bool verifyCode();

    const std::string RGBLuaHeader = "function set_color()\n"
                                     "\t--output pixel will get the values stored in color[\"r\"], color[\"g\"], color[\"b\"] and color[\"a\"]\n"
                                     "\t--input pixel's value is stored in params[\"val\"]";
    const size_t RGBLuaHeaderSize = RGBLuaHeader.size();
    const std::string G16LuaHeader = "function set_color()\n"
                                     "\t--output pixel will get the value stored in color[\"value\"]\n"
                                     "\t--input pixel's value is stored in params[\"val\"]";
    const size_t G16LuaHeaderSize = G16LuaHeader.size();
    const std::string RGBLuaFooter = "end";
    const size_t RGBLuaFooterSize = RGBLuaFooter.size();
    const std::string G16LuaFooter = "end";
    const size_t G16LuaFooterSize = G16LuaFooter.size();
    const std::string CSVLuaHeader = "function set_color()\n"
                                     "--output shapes will be defined by the following values:\n"
                                     "\t--output shape will have its color stored in style[\"r\"], style[\"g\"], style[\"b\"] and style[\"a\"]\n"
                                     "\t--output shape will have its center color stored in style[\"center_r\"], style[\"center_g\"], style[\"center_b\"] and style[\"center_a\"]\n"
                                     "\t--output shape will have its shape type stored in style[\"type\"], accepted values are \"Circle\", \"EmptyCircle\", \"Square\", \"EmptySquare\"\n"
                                     "\t--output shape will have its size stored in style[\"size\"]\n"
                                     "\t--CSV attributes are stored in params[\"your_attribute_name\"]";
    const size_t CSVLuaHeaderSize = CSVLuaHeader.size();
    const std::string CSVLuaFooter = "end";
    const size_t CSVLuaFooterSize = CSVLuaFooter.size();
    const std::string GeoJsonLuaHeader = "function set_color()\n"
                                         "\t--output shapes will get the color stored in color[\"r\"], color[\"g\"], color[\"b\"] and color[\"a\"]\n"
                                         "\t--shape's geometry type is stored in shape_type, possible values are : \"Point\", \"MultiPoint\", \"LineString\", \"MultiLineString\", \"Polygon\" and \"MultiPolygon\"\n"
                                         "\t--JSON attributes for a shape are stored in params[\"your_attribute_name\"]\n"
                                         "\t--the syntax for getting sub-properties in JSON objects is : params[\"your_attribute_name\"][\"sub-property\"][\"another_sub-property\"]";
    const size_t GeoJsonLuaHeaderSize = GeoJsonLuaHeader.size();
    const std::string GeoJsonLuaFooter = "end";
    const size_t GeoJsonLuaFooterSize = GeoJsonLuaFooter.size();
    const std::string GpkgLuaHeader = "function set_color()\n"
                                         "\t--output shapes will get the color stored in color[\"r\"], color[\"g\"], color[\"b\"] and color[\"a\"]\n"
                                         "\t--shape's geometry type is stored in shape_type, possible values are : \"Point\", \"MultiPoint\", \"LineString\", \"MultiLineString\", \"Polygon\" and \"MultiPolygon\"\n"
                                         "\t--attributes for a shape are stored in params[\"your_attribute_name\"]\n"
                                         "\t--all attributes will be queried as strings; if you wish to use them as numbers, use the tonumber() Lua function";
    const size_t GpkgLuaHeaderSize = GpkgLuaHeader.size();
    const std::string GpkgLuaFooter = "end";
    const size_t GpkgLuaFooterSize = GpkgLuaFooter.size();


};

#endif // LUACODEWINDOW_H
