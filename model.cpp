#include "model.h"
#include "database/database.h"
#include <QDataStream>
#include <QFile>
#include <QtSql>
#include <database/addsernums.h>

Model* Model::m_instance = nullptr;

void Add(const Record& record)
{
    static bool fl{};
    if (!fl) {
        fl = true;
        DataBase::createTableOrd();
        DataBase::createTableSn();
    }
    //AddSerNums add;
    //add.addEncSerNums(record.regul().id, record.date(), record.order(), record.orderDate(), record.count());
    int id = 0;
    QSqlTableModel m;
    do {
        m.setTable(TABLE_ORDER);
        if (!m.select())
            break;
        m.setFilter(QString(TORD_NUM + "=%1 AND " + TORD_DATE_ORDER + "='%2' AND " + TORD_ADJ + "='%3' AND " + TORD_DATE_CREATION + "='%4'")
                        .arg(record.order())
                        .arg(record.orderDate().toString("yyyy-MM-dd"))
                        .arg(record.regul().id)
                        .arg(record.date().toString("yyyy-MM-dd")));

        if (m.rowCount()) {
            id = m.record(0).value(TORD_KEY).toInt(); //, m.record(0).value(TABLE_ORD_CTY).toInt() };
            m.setData(m.index(0, 5), m.data(m.index(0, 5)).toInt() + record.count());
            m.submit();
        } else {
            QSqlRecord r(m.record());
            r.setValue(1, record.regul().id);
            r.setValue(2, record.date());
            r.setValue(3, record.order());
            r.setValue(4, record.orderDate());
            r.setValue(5, record.count());
            if (!m.insertRecord(-1, r))
                break;
            if (!m.select())
                break;
            if (m.rowCount())
                id = m.record(0).value(TORD_KEY).toInt(); //, m.record(0).value(TABLE_ORD_CTY).toInt() };
        }
    } while (0);
    {
        if (!id) {
            qDebug() << "orderId" << id;
            //QMessageBox::warning(this, "", "order == 0");
            return;
        }

        int monthCount = 0;
        { // monthCount
            QSqlQueryModel m;
            QDate date{ record.date().year(), record.date().month(), 1 };
            do {
                QSqlQuery q(QString("SELECT SUM(" + TORD_COUNT + ") FROM " + TABLE_ORDER
                    + " WHERE " + TORD_ADJ + " = '%1' AND " + TORD_DATE_CREATION + " BETWEEN '%2' AND '%5'")
                                .arg(record.regul().id)
                                .arg(date.toString("yyyy-MM-dd"))
                                .arg(date.addMonths(1).addDays(-1).toString("yyyy-MM-dd")));
                if (!q.exec())
                    break;
                m.setQuery(q);
                monthCount = m.record(0).value(0).toInt();
            } while (0);
        }

        m.setTable(TABLE_ENC_SER_NUM);
        m.select();

        monthCount = record.sernum();

        int min = Record::toSerNum(record.regul().id, record.date(), monthCount);
        int max = Record::toSerNum(record.regul().id, record.date(), monthCount + record.count() - 1);

        auto key{ record.encodedSernumsV() };
        QVector<int> test;
        test.reserve(record.count());
        for (int i = min; i <= max; ++i) {
            test << i;
            QSqlRecord r(m.record());
            r.setValue(0, i);
            r.setValue(1, monthCount++);
            r.setValue(2, id);
            if (!m.insertRecord(0, r)) {
                qDebug() << "SN" << i << id << m.lastError();
                return;
            }
        }
        std::sort(test.begin(), test.end());
        if (key != test) {
            qDebug() << key;
            qDebug() << test;
            exit(88);
        }
    }
}

void Model::save()
{

    QFile file("database.bin");
    if (m_edited && file.open(QIODevice::WriteOnly)) {
        QDataStream stream(&file);
        stream << m_data;
    }
    //    {
    //        QFile file("database.cache");
    //        if (m_edited && file.open(QIODevice::WriteOnly)) {
    //            QDataStream stream(&file);
    //            stream << m_encSerNumRowsCache;
    //            stream << m_lastSerNumCache;
    //            stream << m_ordersCache;
    //        }
    //    }
}

void Model::restore()
{
    QFile file("database.bin");
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream stream(&file);
        stream >> m_data;

        //        std::sort(m_data.begin(), m_data.end(), [](const Record& r1, const Record& r2) {
        //            return std::tuple { r1.regul().id, r1.date().year(), r1.date().month(), r1.sernum() }
        //            < std::tuple { r2.regul().id, r2.date().year(), r2.date().month(), r2.sernum() };
        //        });

        //        QFile file("database.cache");
        //        if (file.open(QIODevice::ReadOnly)) {
        //            QDataStream stream(&file);
        //            stream >> m_encSerNumRowsCache;
        //            stream >> m_lastSerNumCache;
        //            stream >> m_ordersCache;
        //        } else {
        for (int i = 0; i < m_data.size(); ++i) {
            update(m_data[i], i);
        }
        //        }
    }
}

Model::Model(QObject* parent)

    : QAbstractTableModel(parent)
    , m_headerData{
        "Исполнитель",
        "Дата",
        "Кол-во в мес.",
        "Серийные № (код)",
        "Заказ",
        "Дата Заказа"
    }
{
    restore();
    m_instance = this;
}

Model::~Model()
{
    m_instance = nullptr;
    save();
}

void Model::addRecord(const Record& record)
{
    if (!m_instance)
        return;
    m_instance->m_edited = true;
    m_instance->beginInsertRows({}, m_instance->m_data.count(), m_instance->m_data.count());
    const int index = m_instance->m_data.size();
    m_instance->m_data.append(record); ////////
    m_instance->m_data.last().setId(index);
    m_instance->update(record, index);
    m_instance->endInsertRows();
}

void Model::update(const Record& record, int /*index*/)
{
    Add(record);
    //    for (int sn : record.encodedSernumsV()) {
    //        m_encSerNumRowsCache[sn] = index;
    //    }
    //    m_lastSerNumCache[{ record.regul().id, { record.date().year(), record.date().month(), 1 } }] += record.count();
    //    m_ordersCache[{ record.order(), record.orderDate() }] = index;
    //    save();
}

Record Model::getRecord(int id)
{
    if (!m_instance)
        return {};
    return m_instance->m_data.value(id);
}

int Model::getIndex(int encSn)
{
    if (!m_instance)
        return -1;
    return m_instance->m_encSerNumRowsCache.value(encSn, -1);
}

int Model::getLastSerNum(int regId, const QDate& date)
{
    if (!m_instance)
        return -1;
    return m_instance->m_lastSerNumCache.value({ regId, { date.year(), date.month(), 1 } }, 0) + 1;
}

int Model::getOrderRow(int order, const QDate& date)
{
    if (!m_instance)
        return -1;
    return m_instance->m_ordersCache.value({ order, date }, -1);
}

int Model::rowCount(const QModelIndex& /*parent*/) const
{
    return m_data.count();
}

int Model::columnCount(const QModelIndex& /*parent*/) const
{
    return 6;
}

QVariant Model::data(const QModelIndex& index, int role) const
{
    const int row{ index.row() };
    switch (role) {
    case Qt::ToolTipRole:
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return m_data[row].regul().name;
        case 1:
            return m_data[row].date().toString("dd MMMM yyyyг.");
        case 2:
            return "(" + QString::number(m_data[row].count()) + ")\n" + m_data[row].sernums();
        case 3:
            return m_data[row].encodedSernums();
        case 4:
            return m_data[row].order();
        case 5:
            return m_data[row].orderDate();
        }
    default:
        return {};
    }
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return m_headerData[section];
        //return QAbstractTableModel::headerData(section, orientation, role);
    } else {
        return QAbstractTableModel::headerData(section, orientation, role);
    }
}

bool Model::insertRows(int /*row*/, int /*count*/, const QModelIndex& /*parent*/)
{
    return false;
}

bool Model::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, count);
    for (int r = 0; r < count; ++r) {
        m_data.removeAt(row + r);
    }
    save();
    endRemoveRows();
    return true;
}
