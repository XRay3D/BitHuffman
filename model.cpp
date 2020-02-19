#include "model.h"
#include <QDataStream>
#include <QFile>

Model* Model::m_instance = nullptr;

void Model::save()
{
    QFile file("database.bin");
    if (m_edited && file.open(QIODevice::WriteOnly)) {
        QDataStream stream(&file);
        stream << m_data;
    }
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
        for (int i = 0; i < m_data.size(); ++i) {
            update(m_data[i], i);
        }
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
    for (int sn : record.encodedSernumsV()) {
        m_dataKey[sn] = index;
    }
    m_lastSerNum[{ record.regul().id, { record.date().year(), record.date().month(), 1 } }] += record.count();
    save();
}

Record Model::getRecord(int id)
{
    return m_instance->m_data.value(id);
}

int Model::getIndex(int encSn)
{
    return m_instance->m_dataKey.value(encSn, -1);
}

int Model::getLastSerNum(int regId, const QDate& date)
{
    return m_instance->m_lastSerNum.value({ regId, { date.year(), date.month(), 1 } }, 0) + 1;
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
