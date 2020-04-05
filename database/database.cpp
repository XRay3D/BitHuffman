#include "database.h"

#include <QMessageBox>
#include <model.h>

DataBase::DataBase(QObject* parent)
    : QObject(parent)
{
}

DataBase::~DataBase() { closeDataBase(); }

/* Методы для подключения к базе данных */
bool DataBase::connectToDataBase()
{
    /* Перед подключением к базе данных производим проверку на её существование.
     * В зависимости от результата производим открытие базы данных или её восстановление
     */
    if (QFile(DATABASE_NAME).exists()) {
        return openDataBase();
    } else {
        return restoreDataBase();
    }
}

/* Методы восстановления базы данных */
bool DataBase::restoreDataBase()
{
    if (openDataBase()) {
        if (createTables()) {
            Model(); // store data in DB from old binary Data
            return true;
        }
    }
    closeDataBase();
    qDebug() << "Не удалось восстановить базу данных";
    return false;
}

/* Метод для открытия базы данных */
bool DataBase::openDataBase()
{
    /* База данных открывается по заданному пути
     * и имени базы данных, если она существует
     */

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setHostName(DATABASE_HOSTNAME);
    db.setDatabaseName(DATABASE_NAME);
    if (db.open()) {
        // optimization
        if (1) {
            QSqlQuery q("PRAGMA page_size = 4096", db);
            if (!q.exec())
                qDebug() << q.lastError().text();
        }
        if (1) {
            QSqlQuery q("PRAGMA cache_size = 16384", db);
            if (!q.exec())
                qDebug() << q.lastError().text();
        }
        if (1) {
            QSqlQuery q("PRAGMA temp_store = MEMORY", db);
            if (!q.exec())
                qDebug() << q.lastError().text();
        }
        // performance
        if (1) {
            QSqlQuery q("PRAGMA journal_mode = OFF", db);
            if (!q.exec())
                qDebug() << q.lastError().text();
        }
        if (1) {
            QSqlQuery q("PRAGMA locking_mode = EXCLUSIVE", db);
            if (!q.exec())
                qDebug() << q.lastError().text();
        }
        if (1) {
            QSqlQuery q("PRAGMA synchronous = OFF", db);
            if (!q.exec())
                qDebug() << q.lastError().text();
        }
        return true;
    } else {
        return false;
    }
}

/* Методы закрытия базы данных */
void DataBase::closeDataBase()
{
    db.close();
    QFile(DATABASE_NAME).remove();
}

/* Метод для создания таблицы в базе данных */
bool DataBase::createTables()
{
    /* В данном случае используется формирование сырого SQL-запроса
     * с последующим его выполнением.
     */

    do {
        //                if (QSqlQuery query; !query.exec("CREATE TABLE " + TABLE
        //                        + " ( id INTEGER PRIMARY KEY AUTOINCREMENT, "
        //                        + TABLE_DATE + " DATE NOT NULL,"
        //                        + TABLE_TIME + " TIME NOT NULL,"
        //                        + TABLE_RANDOM + " INTEGER NOT NULL,"
        //                        + TABLE_MESSAGE + " VARCHAR(255) NOT NULL )")) {
        //                    qDebug() << "DataBase: error of create " << TABLE;
        //                    qDebug() << query.lastError().text();
        //                    return false;
        //                }

        if (!createTableAdj())
            break;

        if (!createTableDep())
            break;

        if (!createTableOrd())
            break;

        if (!createTableSn())
            break;

        return true;
    } while (0);
    db.close();
    QFile(DATABASE_NAME).remove();
    return false;
}

bool DataBase::createTableAdj()
{
    dropTable(TABLE_ADJ);

    if (QSqlQuery query; !query.exec("CREATE TABLE " + TABLE_ADJ
            + " ( " //id INTEGER PRIMARY KEY AUTOINCREMENT, "
            + TADJ_ID_KEY + " INTEGER PRIMARY KEY,"
            + TADJ_DEP + " INTEGER NOT NULL,"
            + TADJ_NAME + " VARCHAR(255) NOT NULL"
                          " )")) {
        qDebug() << "DataBase: error of create " << TABLE_ADJ;
        qDebug() << query.lastError().text();
        return false;
    }
    {
        QSqlTableModel m;
        m.setTable(TABLE_ADJ);
        m.select();
        QSqlRecord r(m.record());
        {
            r.setValue(TADJ_ID_KEY, 21);
            r.setValue(TADJ_DEP, 2);
            r.setValue(TADJ_NAME, "Митькин Константин Георгиевич");
            m.insertRecord(m.rowCount(), r);
        }
        {
            r.setValue(TADJ_ID_KEY, 22);
            r.setValue(TADJ_DEP, 2);
            r.setValue(TADJ_NAME, "Дубровин Геннадий Васильевич");
            m.insertRecord(m.rowCount(), r);
        }
        {
            r.setValue(TADJ_ID_KEY, 23);
            r.setValue(TADJ_DEP, 2);
            r.setValue(TADJ_NAME, "Изотов Владислав Юрьевич");
            m.insertRecord(m.rowCount(), r);
        }
        {
            r.setValue(TADJ_ID_KEY, 24);
            r.setValue(TADJ_DEP, 2);
            r.setValue(TADJ_NAME, "Феоктистов Дмитрий Александрович");
            m.insertRecord(m.rowCount(), r);
        }
        {
            r.setValue(TADJ_ID_KEY, 25);
            r.setValue(TADJ_DEP, 2);
            r.setValue(TADJ_NAME, "Доценко Дмитрий Иванович");
            m.insertRecord(m.rowCount(), r);
        }
        {
            r.setValue(TADJ_ID_KEY, 26);
            r.setValue(TADJ_DEP, 2);
            r.setValue(TADJ_NAME, "Беляков Михаил Михайлович");
            m.insertRecord(m.rowCount(), r);
        }
        {
            r.setValue(TADJ_ID_KEY, 27);
            r.setValue(TADJ_DEP, 2);
            r.setValue(TADJ_NAME, "Толстунов Максим Евгеньевич");
            m.insertRecord(m.rowCount(), r);
        }
    }
    return true;
}

bool DataBase::createTableDep()
{
    dropTable(TABLE_DEPARTMENT);

    if (QSqlQuery query; !query.exec("CREATE TABLE " + TABLE_DEPARTMENT
            + " ( " //id INTEGER PRIMARY KEY AUTOINCREMENT, "
            + TDEP_NUMBER + " INTEGER PRIMARY KEY," //NOT NULL UNIQUE,"
            + TDEP_NAME + " VARCHAR(255) NOT NULL"
                          " )")) {
        qDebug() << "DataBase: error of create " << TABLE_DEPARTMENT;
        qDebug() << query.lastError().text();
        return false;
    }

    {
        QSqlTableModel m;
        m.setTable(TABLE_DEPARTMENT);
        m.select();
        QSqlRecord r(m.record());
        r.setValue(TDEP_NUMBER, 2);
        r.setValue(TDEP_NAME, "РУ-2");
        m.insertRecord(m.rowCount(), r);
    }

    return true;
}

bool DataBase::createTableOrd()
{
    dropTable(TABLE_ORDER);

    if (QSqlQuery query; !query.exec("CREATE TABLE " + TABLE_ORDER + " ( "
            + TORD_KEY + " INTEGER PRIMARY KEY AUTOINCREMENT,"
            + TORD_ADJ + " INTEGER NOT NULL,"
            + TORD_DATE_CREATION + " DATE NOT NULL,"
            + TORD_NUM + " INTEGER NOT NULL,"
            + TORD_DATE_ORDER + " DATE NOT NULL,"
            + TORD_COUNT + " INTEGER NOT NULL"
                           " )")) {
        qDebug() << "DataBase: error of create " << TABLE_ADJ;
        qDebug() << query.lastError().text();
        return false;
    }
    return true;
}

bool DataBase::createTableSn()
{
    dropTable(TABLE_ENC_SER_NUM);

    if (QSqlQuery query; !query.exec("CREATE TABLE " + TABLE_ENC_SER_NUM + " ( "
            + TESN_ESN_KEY + " INTEGER PRIMARY KEY,"
            + TESN_MONTH_COUNTER + " INTEGER NOT NULL,"
            + TESN_ORDER + " INTEGER NOT NULL)")) {
        qDebug() << "DataBase: error of create " << TABLE_ENC_SER_NUM;
        qDebug() << query.lastError().text();
        return false;
    }

    return true;
}

bool DataBase::dropTable(const QString& name)
{
    QSqlQuery query;
    query.setForwardOnly(true);
    query.prepare("DROP TABLE IF EXISTS " + name);
    if (!query.exec()) {
        qDebug() << "SQL QUERY ERROR:" << query.lastError().text();
        return false;
    }
    return true;
}

// Метод для вставки записи в базу данных
bool DataBase::inserIntoTable(const QVariantList& /*data*/)
{
    /* Запрос SQL формируется из QVariantList,
 * в который передаются данные для вставки в таблицу.
 * */
    QSqlQuery query;
    /* В начале SQL запрос формируется с ключами,
 * которые потом связываются методом bindValue
 * для подстановки данных из QVariantList
 * */
    //    query.prepare("INSERT INTO " TABLE " ( " TABLE_DATE ", " TABLE_TIME ", " TABLE_RANDOM ", " TABLE_MESSAGE " ) "
    //                  "VALUES (:Date, :Time, :Random, :Message )");
    //    query.bindValue(":Date", data[0].toDate());
    //    query.bindValue(":Time", data[1].toTime());
    //    query.bindValue(":Random", data[2].toInt());
    //    query.bindValue(":Message", data[3].toString());
    //    // После чего выполняется запросом методом exec()
    //    if (!query.exec()) {
    //        qDebug() << "error insert into " << TABLE;
    //        qDebug() << query.lastError().text();
    //        return false;
    //    } else {
    //        return true;
    //    }
    return false;
}

void showError(const QSqlError& err, QWidget* w)
{
    QMessageBox::critical(w, "Unable to initialize Database", "Error initializing database: " + err.text());
}
