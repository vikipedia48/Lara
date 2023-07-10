#include "geopackagewindow.h"
#include "ui_geopackagewindow.h"

GeoPackageWindow::GeoPackageWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeoPackageWindow)
{
    ui->setupUi(this);
}

GeoPackageWindow::~GeoPackageWindow()
{
    delete ui;
}
