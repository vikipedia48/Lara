#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "geotiffwindow.h"
#include "csvwindow.h"
#include "geojsonwindow.h"
#include "geopackagewindow.h"
#include "newcsvwindow.h"
#include "newgeojsonwindow.h"
#include "newgeopackagewindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->currentWindow = nullptr;
    this->setWindowTitle("Lara");
}

MainWindow::~MainWindow()
{
    delete ui;
    delete currentWindow;
}
void MainWindow::on_actionGeoTiff_triggered()
{
    closeCurrentWindow();
    currentWindow = new GeotiffWindow(true);
    ui->widgetWindow->addWidget(currentWindow, 0, Qt::AlignmentFlag::AlignVCenter);
    currentWindow->show();
}


void MainWindow::on_actionCSV_triggered()
{
    closeCurrentWindow();
    currentWindow = new NewCsvWindow();
    ui->widgetWindow->addWidget(currentWindow, 0, Qt::AlignmentFlag::AlignVCenter);
    currentWindow->show();
}

void MainWindow::closeCurrentWindow()
{
    ui->label_welcomeScreen1->hide();
    ui->label_welcomeScreen2->hide();
    ui->label_welcomeScreen3->hide();
    if (currentWindow == nullptr) return;
    currentWindow->close();
    currentWindow = nullptr;
}


void MainWindow::on_actionGeoJson_triggered()
{
    closeCurrentWindow();
    currentWindow = new NewGeoJsonWindow();
    ui->widgetWindow->addWidget(currentWindow, 0, Qt::AlignmentFlag::AlignVCenter);
    currentWindow->show();
}

void MainWindow::on_actionGeoPackage_triggered()
{
    closeCurrentWindow();
    currentWindow = new NewGeoPackageWindow();
    ui->widgetWindow->addWidget(currentWindow, 0, Qt::AlignmentFlag::AlignVCenter);
    currentWindow->show();
}


void MainWindow::on_actionGeoTiff_Legacy_triggered()
{
    closeCurrentWindow();
    currentWindow = new GeotiffWindow();
    ui->widgetWindow->addWidget(currentWindow, 0, Qt::AlignmentFlag::AlignVCenter);
    currentWindow->show();
}


void MainWindow::on_actionCSV_Legacy_triggered()
{
    closeCurrentWindow();
    currentWindow = new CSVWindow();
    ui->widgetWindow->addWidget(currentWindow, 0, Qt::AlignmentFlag::AlignVCenter);
    currentWindow->show();
}


void MainWindow::on_actionGeoJson_Legacy_triggered()
{
    closeCurrentWindow();
    currentWindow = new GeoJsonWindow();
    ui->widgetWindow->addWidget(currentWindow, 0, Qt::AlignmentFlag::AlignVCenter);
    currentWindow->show();
}


void MainWindow::on_actionGeoPackage_Legacy_triggered()
{
    closeCurrentWindow();
    currentWindow = new GeoPackageWindow();
    ui->widgetWindow->addWidget(currentWindow, 0, Qt::AlignmentFlag::AlignVCenter);
    currentWindow->show();
}

