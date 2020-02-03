#include "mainwindow.h"
#include "huffman.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <stdint.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboBox_r->addItems({
        "Иванов",
        "Смирнов",
        "Кузнецов",
        "Попов",
        "Васильев",
        "Петров",
        "Соколов",
        "Михайлов",
        "Новиков",
        "Федоров",
        "Морозов",
        "Волков",
        "Алексеев",
        "Лебедев",
        "Семенов",
        "Егоров",
        "Павлов",
        "Козлов",
        "Степанов",
        "Николаев",
        "Орлов",
        "Андреев",
        "Макаров",
        "Никитин",
        "Захаров",
        "Зайцев",
        "Соловьев",
        "Борисов",
        "Яковлев",
        "Григорьев",
        "Романов",
        "Воробьев",
        "Сергеев",
        "Кузьмин",
        "Фролов",
        "Александров",
        "Дмитриев",
        "Королев",
        "Гусев",
        "Киселев",
        "Ильин",
        "Максимов",
        "Поляков",
        "Сорокин",
        "Виноградов",
        "Ковалев",
        "Белов",
        "Медведев",
        "Антонов",
        "Тарасов",
        "Жуков",
        "Баранов",
        "Филиппов",
        "Комаров",
        "Давыдов",
        "Беляев",
        "Герасимов",
        "Богданов",
        "Осипов",
        "Сидоров",
        "Матвеев",
        "Титов",
        "Марков",
        "Миронов",
    });
    ui->comboBox_m->addItems({
        "Январь",
        "Февраль",
        "Март",
        "Апрель",
        "Май",
        "Июнь",
        "Июль",
        "Август",
        "Сентябрь",
        "Октябрь",
        "Ноябрь",
        "Декабрь",
    });
    try {

        QString testStr("beep boop beer!");
        //We build the tree depending on the string
        htTree* codeTree = buildTree(testStr);
        //We build the table depending on the Huffman tree
        hlTable* codeTable = buildTable(codeTree);

        //We encode using the Huffman table
        encode(codeTable, testStr);
        //We decode using the Huffman tree
        //We can decode string that only use symbols from the initial string
        decode(codeTree, /*encode(codeTable, testStr) */ "0011111010110001001010101100111110001001");
        //Output : 0011 1110 1011 0001 0010 1010 1100 1111 1000 1001
        /*
        00111110
        10110001
        00101010
        11001111
        10001001
        */

    } catch (...) {
        qDebug() << "err";
        exit(0);
    }
    exit(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//#pragma pack(push, 1)
//struct name {
//    union U {
//        struct {
//            unsigned regul : 6;
//            unsigned month : 4;
//            unsigned yar : 3;
//            unsigned serNun : 11;
//            unsigned a : 8;
//        } str;
//        int32_t raw;
//    } u;
//};
//#pragma pack(pop, 1)

void MainWindow::on_pbToSerNum_clicked()
{
    //    qDebug() << sizeof(name);
    //    name n;
    //    memset(&n, 0, sizeof(name));
    //    n.u.str.yar = ui->spinBox_y->value();
    //    n.u.str.regul = ui->comboBox->currentIndex();
    //    n.u.str.month = ui->comboBox_2->currentIndex();
    //    n.u.str.serNun = ui->spinBox_s->value() - 1;
    //    ui->spinBox_r->setValue(n.u.raw);

    int yar = ui->spinBox_y->value() << 11;
    int regul = ui->comboBox_r->currentIndex() << 18;
    int month = ui->comboBox_m->currentIndex() << 14;
    int serNun = (ui->spinBox_s->value() - 1);

    ui->spinBox_r->setValue(yar | regul | month | serNun);
}

void MainWindow::on_pbFromSerNum_clicked()
{
    //    qDebug() << sizeof(name);
    //    name n;
    //    memset(&n, 0, sizeof(name));
    //    n.u.raw = ui->spinBox_r->value();
    //    ui->spinBox_y->setValue(n.u.str.yar);
    //    ui->comboBox->setCurrentIndex(n.u.str.regul);
    //    ui->comboBox_2->setCurrentIndex(n.u.str.month);
    //    ui->spinBox_s->setValue(n.u.str.serNun + 1);

    int raw = ui->spinBox_r->value();

    ui->spinBox_y->setValue((raw >> 11) & 0b111);
    ui->comboBox_r->setCurrentIndex((raw >> 18) & 0b111111);
    ui->comboBox_m->setCurrentIndex((raw >> 14) & 0b1111);
    ui->spinBox_s->setValue((raw & 0b11111111111) + 1);
}
