#include "types.h"

#include <QComboBox>
#include <QFile>
#include <QTextCodec>
#include <stdint.h>

QMap<int, Regul> Regul::regul;
QMap<int, int> Regul::revRegul;

#pragma pack(push, 1)

typedef union {
    struct {
        unsigned ser : 10; //  (serNum - 1); in month
        unsigned dat : 7; // (date.year() - 2020) << OffsetYear;
        unsigned reg : 6; // regId << OffsetRegul;
        unsigned : 9; // regId << OffsetRegul;
    };
    struct {
        int32_t csn;
    };
} SN;

#pragma pack(pop)

Regul::Regul()
    : id(-1)
    , department{}
    , name{}
{
}

Regul::Regul(const QStringList& list)
    : id(list.value(0).toInt())
    , department(list.value(1))
    , name(list.value(2))
{
    if (list.size() < 3) {
        QMessageBox::warning(nullptr,
            "Ошибка",
            "Запись данных в файле \"регулировщики.csv\" не соотвнтствует формату:\n\"№ регулировщика;Отдел;Ф.И.О.\"");
        exit(0);
    }
}

void Regul::load(QComboBox* cbx)
{
    QFile file("регулировщики.csv");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        //in.setCodec(QTextCodec::codecForName("Windows-1251"/*"UTF-8"*/));
        QSet<int> regSet;
        int index = 0;
        while (!in.atEnd()) {
            Regul::regul[index] = Regul{ in.readLine().split(';', QString::SkipEmptyParts) };
            Regul::revRegul[Regul::regul[index].id] = index;
            cbx->addItem(Regul::regul[index].name);
            if (!regSet.contains(Regul::regul.last().id)) {
                regSet.insert(Regul::regul.last().id);
            } else {
                QMessageBox::warning(nullptr,
                    "Ошибка",
                    "В файле \"регулировщики.csv\" содержатся разные записи под одним номером.");
                exit(0);
            }
            //            ui->cbxRegul->addItem(Regul::regul[index].name);

            ++index;
        }
    } else {
        QMessageBox::warning(nullptr,
            "Ошибка",
            "Не удалось загрузить список регулиловщиков из файла:\n\"регулировщики.csv\"");
        exit(0);
    }
}

Regul Regul::fromIndex(int i) { return regul[i]; }

int Regul::toIndex(int id) { return revRegul[id]; }

////////////////////////////////////////////////////////////////////

Record::Record() {}

Record::Record(const Regul& regul, const QDate& date, int sernum, int count, int order, const QDate& orderDate)
    : m_regul(regul)
    , m_date(date)
    , m_sernum(sernum)
    , m_count(count)
    , m_order(order)
    , m_orderDate(orderDate)
    , m_id(0)
{
}

int Record::toSerNum(int regId, const QDate& date, int serNum)
{
    SN sn;
    sn.csn = 0;
    sn.ser = static_cast<unsigned>(serNum - 1);
    sn.dat = static_cast<unsigned>((date.year() - 2020) * 12 + date.month() - 1);
    sn.reg = static_cast<unsigned>(regId);
    return sn.csn; // yar + reg + month + serNun;
}

std::tuple<int, QDate, int> Record::fromSerNum(int raw)
{
    SN sn;
    sn.csn = raw;
    return { static_cast<int>(sn.reg), QDate{ static_cast<int>(sn.dat / 12 + 2020), static_cast<int>(sn.dat % 12 + 1), 1 }, static_cast<int>(sn.ser + 1) };
}

QString Record::sernums() const
{
    if (m_sernums.isEmpty())
        for (int sn = 0; sn < m_count; ++sn) {
            m_sernums += (sn ? "\n" : "") + QString::number(m_sernum + sn);
        }
    return m_sernums;
}

QString Record::encodedSernums() const
{
    if (m_encodedSernums.isEmpty())
        for (int sn = 0; sn < m_count; ++sn) {
            m_encodedSernums += (sn ? "\n" : "") + QString::number(toSerNum(m_regul.id, m_date, m_sernum + sn));
        }
    return m_encodedSernums;
}

QVector<int> Record::encodedSernumsV() const
{
    QVector<int> sernums;
    sernums.reserve(m_count);
    for (int sn = 0; sn < m_count; ++sn) {
        sernums << toSerNum(m_regul.id, m_date, m_sernum + sn);
    }
    std::sort(sernums.begin(),sernums.end());
    return sernums;
}

QString Record::toString() const
{
    return m_encodedSernums;
}
