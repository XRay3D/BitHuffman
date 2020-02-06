#ifndef MODEL_H
#define MODEL_H

#include "types.h"
#include <QAbstractTableModel>

class Model : public QAbstractTableModel {
    Q_OBJECT

    QList<Record> m_data;
    QMap<int, int> m_dataKey;

    const QStringList m_headerData;
    static Model* m_instance;

    void save();
    void restore();

public:
    explicit Model(QObject* parent = nullptr);
    ~Model() override;
    static void addRecord(const Record& record);
    static Record getRecord(int id);
    static int getIndex(int encSn);

signals:

    // QAbstractItemModel interface
public:
    int
    rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool insertRows(int row, int count, const QModelIndex& parent) override;
    bool removeRows(int row, int count, const QModelIndex& parent) override;
};

#endif // MODEL_H
