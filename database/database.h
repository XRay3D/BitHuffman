#ifndef DATABASE_H
#define DATABASE_H

#include <QDate>
#include <QDebug>
#include <QFile>
#include <QObject>
#include <QSql>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

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
constexpr ConstLatin1String TABLE_DEP_NUMBER("DepartmentNumber");
constexpr ConstLatin1String TABLE_DEP_NAME("DepartmentName");

constexpr ConstLatin1String TABLE_SN("TableSerNum");
constexpr ConstLatin1String TABLE_SN_ADJ_ID("AdjusterId");
constexpr ConstLatin1String TABLE_SN_DATE_CREATION("DateOfCreation");
constexpr ConstLatin1String TABLE_SN_MONTH_COUNT("MonthCounter");
constexpr ConstLatin1String TABLE_SN_CODED("CodedSerNumber");
constexpr ConstLatin1String TABLE_SN_ORDER_NUM("OrderNumber");
constexpr ConstLatin1String TABLE_SN_ORDER_DATE("OrderDate");

constexpr ConstLatin1String TABLE_ADJ("TableAdjuster");
constexpr ConstLatin1String TABLE_ADJ_DEP("Department");
constexpr ConstLatin1String TABLE_ADJ_ID("AdjusterId");
constexpr ConstLatin1String TABLE_ADJ_NAME("Name");

constexpr ConstLatin1String TABLE_ORDER("TableOrder");
constexpr ConstLatin1String TABLE_ORD_NUM("OrderNymber");
constexpr ConstLatin1String TABLE_ORD_DATE("OrderDate");

class DataBase : public QObject {
    Q_OBJECT
public:
    explicit DataBase(QObject* parent = 0);
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
    bool createTable();
};

#endif // DATABASE_H
