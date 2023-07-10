#ifndef GEOPACKAGEWINDOW_H
#define GEOPACKAGEWINDOW_H

#include <QWidget>

namespace Ui {
class GeoPackageWindow;
}

class GeoPackageWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GeoPackageWindow(QWidget *parent = nullptr);
    ~GeoPackageWindow();

private:
    Ui::GeoPackageWindow *ui;
};

#endif // GEOPACKAGEWINDOW_H
