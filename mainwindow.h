#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionGeoTiff_triggered();

    void on_actionCSV_triggered();

    void on_actionGeoJson_triggered();

    void on_actionGeoPackage_triggered();

private:
    Ui::MainWindow *ui;
    QWidget* currentWindow;

    void closeCurrentWindow();

};

#endif // MAINWINDOW_H
