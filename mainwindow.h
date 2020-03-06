#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <types.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_pbGen_clicked();
    void on_groupBoxFormat_toggled(bool arg1);
    void on_groupBoxFormatBit_toggled(bool arg1);

private:
    Ui::MainWindow* ui;
    bool skip = false;

    void toEncSerNum();
    void fromEncSerNum();
    void printDialog();
    void tos();
    bool warningOrder();
};
#endif // MAINWINDOW_H
