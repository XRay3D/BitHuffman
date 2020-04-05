#ifndef DATABASE_H
#define DATABASE_H

#include <QDebug>
#include <QObject>
#include <QtSql>
#include <types.h>

void showError(const QSqlError& err, QWidget* w = nullptr);

/* Директивы имен таблицы, полей таблицы и базы данных */
struct ConstLatin1String : public QLatin1String {
    constexpr ConstLatin1String(const char* const s)
        : QLatin1String(s, static_cast<int>(std::char_traits<char>::length(s)))
    {
    }
};

constexpr ConstLatin1String DATABASE_HOSTNAME("DataBase");
constexpr ConstLatin1String DATABASE_NAME("database.db");

constexpr ConstLatin1String TABLE_DEPARTMENT("TableDepartment");
constexpr ConstLatin1String TDEP_NUMBER("DepNumber");
constexpr ConstLatin1String TDEP_NAME("DepName");

constexpr ConstLatin1String TABLE_ENC_SER_NUM("TableEncSerNum");
constexpr ConstLatin1String TESN_ESN_KEY("EsnValueKey");
constexpr ConstLatin1String TESN_MONTH_COUNTER("EsnMonthCounter");
constexpr ConstLatin1String TESN_ORDER("EsnOrder");

constexpr ConstLatin1String TABLE_ADJ("TableAdjuster");
constexpr ConstLatin1String TADJ_DEP("AdjDepartment");
constexpr ConstLatin1String TADJ_ID_KEY("AdjIdKey");
constexpr ConstLatin1String TADJ_NAME("AdjName");

constexpr ConstLatin1String TABLE_ORDER("TableOrder");
constexpr ConstLatin1String TORD_KEY("OrdKey");
constexpr ConstLatin1String TORD_ADJ("OrdAdjId");
constexpr ConstLatin1String TORD_NUM("OrdNymber");
constexpr ConstLatin1String TORD_DATE_ORDER("OrdDate");
constexpr ConstLatin1String TORD_DATE_CREATION("OrdDateCr");
constexpr ConstLatin1String TORD_COUNT("OrdCount");

class DataBase : public QObject {
    Q_OBJECT
    friend class MainWindow;
    friend void Add(const Record&);

public:
    explicit DataBase(QObject* parent = nullptr);
    ~DataBase();
    /* Методы для непосредственной работы с классом
     * Подключение к базе данных и вставка записей в таблицу
     * */
    bool connectToDataBase();
    bool inserIntoTable(const QVariantList& data);

private:
    // Сам объект базы данных, с которым будет производиться работа
    QSqlDatabase db;

private:
    /* Внутренние методы для работы с базой данных
     * */
    bool openDataBase();
    bool restoreDataBase();
    void closeDataBase();
    bool createTables();

    static bool createTableAdj();
    static bool createTableDep();
    static bool createTableOrd();
    static bool createTableSn();
    static bool dropTable(const QString& name);
};

#endif // DATABASE_H
