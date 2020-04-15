#include "addsernums.h"
#include "database.h"
#include "ui_addsernums.h"
#include <types.h>

AddSerNums::AddSerNums(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AddSerNums)
{
    ui->setupUi(this);

    ui->deCreation->setDate(QDate::currentDate());
    ui->deOrder->setDate(QDate::currentDate());
    // ui->sbxOrderCount->setValue(10);
    //  ui->sbxOrder->setValue(1780);
    //  ui->dateOrder->setDate(QDate(2020, 1, 27));

    orderModel.setTable(TABLE_ORDER);
    orderModel.select();

    { // Initialize the combo box:
        auto model = new QSqlTableModel(ui->cbxAdjuster);
        model->setTable(TABLE_ADJ);
        model->select();
        ui->cbxAdjuster->setModel(model);
        ui->cbxAdjuster->setModelColumn(model->fieldIndex(TADJ_NAME));
    }

    testOrder();
}

AddSerNums::~AddSerNums()
{
    delete ui;
}

int AddSerNums::adjId()
{
    auto m = ui->cbxAdjuster->model();
    return m->data(m->index(ui->cbxAdjuster->currentIndex(), 0)).toInt();
}

int AddSerNums::monthCount()
{
    QSqlQueryModel m;
    QDate date { ui->deCreation->date().year(), ui->deCreation->date().month(), 1 };
    do {
        QSqlQuery q(QString("SELECT SUM(" + TORD_COUNT + ") FROM " + TABLE_ORDER
            + " WHERE " + TORD_ADJ + " = '%1' AND " + TORD_DATE_CREATION + " BETWEEN '%2' AND '%5'")
                        .arg(adjId())
                        .arg(date.toString("yyyy-MM-dd"))
                        .arg(date.addMonths(1).addDays(-1).toString("yyyy-MM-dd")));
        if (!q.exec())
            break;
        m.setQuery(q);
        return m.record(0).value(0).toInt();
    } while (0);
    qDebug() << m.query().lastError().text();
    return 0;
}

void AddSerNums::testOrder()
{
    qDebug() << Q_FUNC_INFO;
    orderModel.setFilter(QString(TORD_NUM + "='%1' AND " + TORD_DATE_ORDER + "='%2'")
                             .arg(ui->sbxOrderNumber->value())
                             .arg(ui->deOrder->date().toString("yyyy-MM-dd")));

    if (orderModel.rowCount()) {
        setOrderFilter(orderModel.filter());
        {
            auto m { ui->cbxAdjuster->model() };
            auto mix { m->match(m->index(0, 0), Qt::DisplayRole, orderModel.record(0).value(TORD_ADJ).toInt(), 1, 0) };
            if (!mix.isEmpty()) {
                ui->cbxAdjuster->setCurrentIndex(mix.first().row());
                ui->cbxAdjuster->setEnabled(false);
            }
        }
        {
            ui->deCreation->setDate(orderModel.record(0).value(TORD_DATE_CREATION).toDate());
            ui->deCreation->setEnabled(false);
        }
        {
            ui->sbxOrderCount->setMinimum(orderModel.record(0).value(TORD_COUNT).toInt());
        }
        QMessageBox::warning(this, "", "Данный заказ уже есть в базе!\nВозможно лишь увеличение кол-ва в заазе.");
    } else {
        setOrderFilter("");
        ui->sbxOrderCount->setMinimum(1);
        ui->sbxOrderCount->setValue(1);
        ui->cbxAdjuster->setEnabled(true);
        ui->deCreation->setEnabled(true);
    }
}

void AddSerNums::addSerNums(int orderId, const int count)
{
    // add codded serial numbers
    int counter {};
    int monthCount = 1;
    QSqlTableModel m;
    m.setTable(TABLE_ENC_SER_NUM);
    m.select();
    auto r { m.record() };
    while (counter < count) {
        int esnKey = Record::toSerNum(adjId(), ui->deCreation->date(), monthCount);
        r.setValue(TESN_ESN_KEY, esnKey);
        r.setValue(TESN_MONTH_COUNTER, monthCount);
        r.setValue(TESN_ORDER, orderId);
        ++monthCount;
        if (!m.insertRecord(-1, r))
            qDebug() << "SN" << esnKey << m.lastError();
        else
            ++counter;
    }
}

void AddSerNums::accept()
{
    auto monthCount_ { monthCount() };
    if (monthCount_ + ui->sbxOrderCount->value() - ui->sbxOrderCount->minimum() > 1024) {
        QMessageBox::warning(this, "", "Превышено кол-во изделий в месяц для одного регулировщика!!");
        return;
    }
    int id = 0;
    if (orderModel.rowCount()) {
        if (ui->sbxOrderCount->value() == ui->sbxOrderCount->minimum()) {
            QMessageBox::warning(this, "", "Кол-во совпадает!!");
            return;
        } else {
            auto index { orderModel.index(0, orderModel.fieldIndex(TORD_COUNT)) };
            orderModel.setData(index, ui->sbxOrderCount->value());
            orderModel.submit();
            id = orderModel.record(0).value(TORD_KEY).toInt();
            addSerNums(id, ui->sbxOrderCount->value() - ui->sbxOrderCount->minimum());
            //            { // add codded serial numbers
            //                QSqlTableModel m;
            //                m.setTable(TABLE_ENC_SER_NUM);
            //                m.select();
            //                int min = Record::toSerNum(adjId(), ui->deCreation->date(), monthCount_ + 1);
            //                int max = Record::toSerNum(adjId(), ui->deCreation->date(), monthCount_ + ui->sbxOrderCount->value() - ui->sbxOrderCount->minimum());
            //                qDebug() << min << max;
            //                auto r { m.record() };
            //                for (int i = min; i <= max; ++i) {
            //                    r.setValue(TESN_ESN_KEY, i);
            //                    r.setValue(TESN_MONTH_COUNTER, ++monthCount_);
            //                    r.setValue(TESN_ORDER, id);
            //                    if (!m.insertRecord(-1, r)) {
            //                        qDebug() << "SN" << i << id << m.lastError();
            //                        return;
            //                    }
            //                }
            //            }
            setOrderFilter(orderModel.filter());
            QDialog::accept();
        }
    } else {
        qDebug("accept() new");
        { // add order
            auto r { orderModel.record() };
            r.setValue(TORD_ADJ, adjId());
            r.setValue(TORD_COUNT, ui->sbxOrderCount->value());
            r.setValue(TORD_DATE_CREATION, ui->deCreation->date());
            r.setValue(TORD_DATE_ORDER, ui->deOrder->date());
            r.setValue(TORD_NUM, ui->sbxOrderNumber->value());
            if (!orderModel.insertRecord(-1, r)) {
                qDebug() << "insertRecord" << orderModel.lastError();
                return;
            }
            if (!orderModel.select()) {
                qDebug() << "submit" << orderModel.lastError();
                return;
            }
            if (orderModel.rowCount()) {
                qDebug() << (id = orderModel.record(0).value(TORD_KEY).toInt());
            }
            if (!id) {
                qDebug() << "orderId" << id << orderModel.lastError();
                return;
            }
        }
        addSerNums(id, ui->sbxOrderCount->value());
        setOrderFilter(orderModel.filter());
        QDialog::accept();
    }
}

void AddSerNums::on_deOrder_dateChanged(const QDate& /*date*/) { testOrder(); }
void AddSerNums::on_sbxOrderNumber_valueChanged(int /*arg1*/) { testOrder(); }
