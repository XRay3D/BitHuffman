#ifndef ADDADJUSTER_H
#define ADDADJUSTER_H

#include <QDialog>

namespace Ui {
class AddAdjuster;
}

class QSqlRelationalTableModel;

class AddAdjuster : public QDialog {
    Q_OBJECT

public:
    explicit AddAdjuster(QWidget* parent = nullptr);
    ~AddAdjuster();

private slots:
    void on_pushButtonAdd_clicked();
    void on_pushButtonDel_clicked();
    void on_comboBoxDepartment_currentIndexChanged(int index);

private:
    Ui::AddAdjuster* ui;
    QSqlRelationalTableModel* model;
    int depIdx;
    int getDepIdx();
};

#endif // ADDADJUSTER_H
