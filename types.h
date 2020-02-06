#ifndef REGUL_H
#define REGUL_H

#include <QDate>
#include <QDebug>
#include <QMap>
#include <QMessageBox>
#include <QStringList>

enum {
    OffsetYear = 10,
    OffsetMonth = 13,
    OffsetRegul = 17
};

class QComboBox;

class Regul {
    static QMap<int, Regul> regul;
    static QMap<int, int> revRegul;

    friend QDataStream& operator>>(QDataStream& stream, Regul& r)
    {
        stream >> r.id;
        stream >> r.department;
        stream >> r.name;
        return stream;
    }

    friend QDataStream& operator<<(QDataStream& stream, const Regul& r)
    {
        stream << r.id;
        stream << r.department;
        stream << r.name;
        return stream;
    }

public:
    Regul();
    Regul(const QStringList& list);
    static void load(QComboBox* cbx);
    static Regul fromIndex(int i);
    static int toIndex(int id);

    int id;
    QString department;
    QString name;
};

class Record {

    friend QDataStream& operator>>(QDataStream& stream, Record& r)
    {
        stream >> r.m_regul;
        stream >> r.m_date;
        stream >> r.m_sernum;
        stream >> r.m_count;
        stream >> r.m_order;
        stream >> r.m_orderDate;
        stream >> r.m_id;
        return stream;
    }

    friend QDataStream& operator<<(QDataStream& stream, const Record& r)
    {
        stream << r.m_regul;
        stream << r.m_date;
        stream << r.m_sernum;
        stream << r.m_count;
        stream << r.m_order;
        stream << r.m_orderDate;
        stream << r.m_id;
        return stream;
    }

    Regul m_regul;
    QDate m_date;
    int m_sernum;
    int m_count;
    int m_order;
    QDate m_orderDate;
    int m_id = -1;
    mutable QString m_sernums;
    mutable QString m_encodedSernums;

public:
    Record();
    Record(
        const Regul& regul,
        const QDate& date,
        int sernum,
        int count,
        int order,
        const QDate& orderDate);

    static int toSerNum(int regId, const QDate& m_date, int serNum);
    static std::tuple<int, QDate, int> fromSerNum(int raw);

    inline Regul regul() const { return m_regul; }
    inline QDate date() const { return m_date; }
    inline int sernum() const { return m_sernum; }
    inline int count() const { return m_count; }
    inline int order() const { return m_order; }
    inline QDate orderDate() const { return m_orderDate; }
    inline int isValid() const { return m_id != -1; }
    inline int id() const { return m_id; }
    inline void setId(int id) { m_id = id; }
    QString sernums() const;
    QString encodedSernums() const;
    QVector<int> encodedSernumsV() const;
    QString toString() const;
};

#endif // REGUL_H
