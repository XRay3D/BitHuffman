#ifndef ADDDEPARTMENT_H
#define ADDDEPARTMENT_H

#include <QDialog>

namespace Ui {
class AddDepartment;
}

class QSqlTableModel;

class AddDepartment : public QDialog {
    Q_OBJECT

public:
    explicit AddDepartment(QWidget* parent = nullptr);
    ~AddDepartment();

private slots:
    void on_pushButtonAdd_clicked();
    void on_pushButtonDel_clicked();

private:
    Ui::AddDepartment* ui;
    QSqlTableModel* model;
};

#endif // ADDDEPARTMENT_H
