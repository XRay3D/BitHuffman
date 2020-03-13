#include "model.h"
#include "database/database.h"
#include <QDataStream>
#include <QFile>
#include <QtSql>

Model* Model::m_instance = nullptr;

void Add(const Record& record)
{
    static bool fl{};
    if (fl) {
        fl = true;
        QSqlQuery q;
        //        q.prepare("DELETE FROM " + TABLE_SN + " WHERE id > 0");
        //        if (!q.exec())
        //            qDebug() << "SQL QUERY ERROR:" << q.lastError().text();

        //        q.prepare("DELETE FROM " + TABLE_ORDER + " WHERE id > 0");
        //        if (!q.exec())
        //            qDebug() << "SQL QUERY ERROR:" << q.lastError().text();

        qDebug() << "DELETE";

        q.setForwardOnly(true);
        q.prepare("DROP TABLE IF EXISTS " + TABLE_SN);
        if (!q.exec())
            qDebug() << "SQL QUERY ERROR:" << q.lastError().text();

        if (QSqlQuery query; !query.exec("CREATE TABLE " + TABLE_SN
                + " ( id INTEGER PRIMARY KEY AUTOINCREMENT, "
                + TABLE_SN_ADJ_ID + " INTEGER NOT NULL,"
                + TABLE_SN_DATE_CREATION + " DATE NOT NULL,"
                + TABLE_SN_MONTH_COUNT + " INTEGER NOT NULL,"
                + TABLE_SN_CODED + " INTEGER NOT NULL UNIQUE,"
                + TABLE_SN_ORDER_NUM + " INTEGER NOT NULL,"
                + TABLE_SN_ORDER_DATE + " INTEGER NOT NULL"
                                        " )")) {
            qDebug() << "DataBase: error of create " << TABLE_SN;
            qDebug() << query.lastError().text();
        }
    }
    int id;
    {
        QSqlQuery q;
        q.setForwardOnly(true);
        q.prepare("SELECT * FROM " + TABLE_ORDER
            + " WHERE " + TABLE_ORD_NUM + " = :ref_id1"
            + " AND " + TABLE_ORD_DATE + " = :ref_id2");
        q.bindValue(":ref_id1", record.order());
        q.bindValue(":ref_id2", record.orderDate());

        if (!q.exec())
            qDebug() << "SQL QUERY ERROR:" << q.lastError().text();

        if /*while*/ (q.next()) {
            qDebug() << (id = q.value(0).toInt()) << q.value(1) << q.value(2).toDate().toString("dd.MM.yyyy");
        } else {
            if (!q.prepare("INSERT INTO " + TABLE_ORDER + "(" + TABLE_ORD_NUM + ", " + TABLE_ORD_DATE + ") VALUES(?, ?)"))
                qDebug() << q.lastError().text();
            q.addBindValue(record.order());
            q.addBindValue(record.orderDate());
            if (!q.exec())
                qDebug() << q.lastError().text();
        }
    }
    {
        QSqlQuery q;

        for (int sn : record.encodedSernumsV()) {
            if (!q.prepare("INSERT INTO " + TABLE_SN + "("
                    + TABLE_SN_ADJ_ID + ", "
                    + TABLE_SN_DATE_CREATION + ", "
                    + TABLE_SN_MONTH_COUNT + ", "
                    + TABLE_SN_CODED + ", "
                    + TABLE_SN_ORDER_NUM + ", "
                    + TABLE_SN_ORDER_DATE + ") VALUES(?, ?, ?, ?, ?, ?)"))
                qDebug() << q.lastError().text();
            auto [reg, dat, ser] = Record::fromSerNum(sn);
            q.addBindValue(reg);
            q.addBindValue(dat);
            q.addBindValue(ser);
            q.addBindValue(sn);
            q.addBindValue(id);
            q.addBindValue(id);
            if (!q.exec())
                qDebug() << q.lastError().text();
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

        std::sort(m_data.begin(), m_data.end(), [](const Record& r1, const Record& r2) {
            return std::tuple{ r1.regul().id, r1.date().year(), r1.date().month(), r1.sernum() }
            < std::tuple{ r2.regul().id, r2.date().year(), r2.date().month(), r2.sernum() };
        });

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
        "Регулировщик",
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

void Model::update(const Record& record, int index)
{
    Add(record);
    for (int sn : record.encodedSernumsV()) {
        m_encSerNumRowsCache[sn] = index;
    }
    m_lastSerNumCache[{ record.regul().id, { record.date().year(), record.date().month(), 1 } }] += record.count();
    m_ordersCache[{ record.order(), record.orderDate() }] = index;
    save();
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
