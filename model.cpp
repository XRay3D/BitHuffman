#include "model.h"
#include <QDataStream>
#include <QFile>

Model* Model::m_instance = nullptr;

void Model::save()
{
    QFile file("database.bin");
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream stream(&file);
        stream << m_data;
        //        m_dataKey.clear();
        //        int index = 0;
        //        for (int i = 0; i < m_data.size(); ++i) {
        //            qDebug() << i;
        //            auto& r = m_data[i];
        //            r.setId(index);
        //            for (int sn : r.encodedSernumsV()) {
        //                m_dataKey[sn] = index;
        //            }
        //            ++index;
        //        }
        stream << m_dataKey;
    }
}

void Model::restore()
{
    QFile file("database.bin");
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream stream(&file);
        stream >> m_data;
        stream >> m_dataKey;
    }
}

Model::Model(QObject* parent)

    : QAbstractTableModel(parent)
    , m_headerData{
        "Регулировщик",
        "Дата",
        "Серийные № (мес)",
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
    m_instance->beginInsertRows({}, m_instance->m_data.count(), m_instance->m_data.count());
    const int index = m_instance->m_data.size();
    m_instance->m_data.append(record); ////////
    m_instance->m_data.last().setId(index);
    for (int sn : m_instance->m_data.last().encodedSernumsV()) {
        m_instance->m_dataKey[sn] = index;
    }
    m_instance->save();
    m_instance->endInsertRows();
}

Record Model::getRecord(int id)
{
    return m_instance->m_data.value(id);
}

int Model::getIndex(int encSn)
{
    return m_instance->m_dataKey.value(encSn, -1);
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
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return m_data[row].regul().name;
        case 1:
            return m_data[row].date().toString("MMMM yyyyг.");
        case 2:
            return m_data[row].sernums();
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
