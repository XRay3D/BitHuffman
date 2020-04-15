#ifndef ADDSERNUMS_H
#define ADDSERNUMS_H

#include <QDialog>
#include <QtSql>

namespace Ui {
class AddSerNums;
}

class AddSerNums : public QDialog {
    Q_OBJECT

public:
    explicit AddSerNums(QWidget* parent = nullptr);
    ~AddSerNums() override;

private:
    Ui::AddSerNums* ui;
    int adjId();

    int monthCount();

    QSqlTableModel orderModel;

    void testOrder();
    void addSerNums(int orderId, const int count);

signals:
    void setOrderFilter(const QString& filter);

    // QDialog interface
public slots:
    void accept() override;

private slots:
    void on_deOrder_dateChanged(const QDate& date);
    void on_sbxOrderNumber_valueChanged(int arg1);
};

#endif // ADDSERNUMS_H
