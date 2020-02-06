#include "mainwindow.h"
#include "model.h"
#include "ui_mainwindow.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    Regul::load(ui->cbxRegul);

    connect(ui->sbxSerNumCoded, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::fromSerNum);

    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &MainWindow::toSerNum);
    connect(ui->cbxRegul, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::toSerNum);
    connect(ui->sbxSerNum, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::toSerNum);

    ui->tableView->setModel(new Model(ui->tableView));
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->tableView->installEventFilter(this);

    connect(ui->tableView, &QTableView::customContextMenuRequested, [this](const QPoint& pos) {
        QMenu menu;
        //        menu.addAction("Печать", [this] {
        //        });
        menu.addAction("Копировать", [this] {
            QClipboard* clipboard = QGuiApplication::clipboard();
            QString text;
            for (auto& index : ui->tableView->selectionModel()->selectedRows()) {
                text += Model::getRecord(index.row()).toString();
            }
            clipboard->setText(text);
        });
        menu.exec(ui->tableView->mapToGlobal(pos));
    });
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    toSerNum();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::toSerNum()
{
    disconnect(ui->sbxSerNumCoded, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::fromSerNum);

    ui->sbxSerNumCoded->setValue(
        Record::toSerNum(Regul::fromIndex(ui->cbxRegul->currentIndex()).id,
            ui->dateEdit->date(),
            ui->sbxSerNum->value()));

    connect(ui->sbxSerNumCoded, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::fromSerNum);
}

void MainWindow::fromSerNum()
{
    disconnect(ui->dateEdit, &QDateEdit::dateChanged, this, &MainWindow::toSerNum);
    disconnect(ui->cbxRegul, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::toSerNum);
    disconnect(ui->sbxSerNum, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::toSerNum);

    auto [regId, date, serNum] = Record::fromSerNum(ui->sbxSerNumCoded->value());
    ui->dateEdit->setDate(date);
    ui->cbxRegul->setCurrentIndex(Regul::toIndex(regId));
    ui->sbxSerNum->setValue(serNum);

    ui->tableView->selectRow(Model::getIndex(ui->sbxSerNumCoded->value()));
    ui->tableView->showRow(Model::getIndex(ui->sbxSerNumCoded->value()));

    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &MainWindow::toSerNum);
    connect(ui->cbxRegul, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::toSerNum);
    connect(ui->sbxSerNum, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::toSerNum);
}

void MainWindow::on_pbGen_clicked()
{
    toSerNum();
    Model::addRecord({ //
        Regul::fromIndex(ui->cbxRegul->currentIndex()),
        ui->dateEdit->date(),
        ui->sbxSerNum->value(),
        ui->sbxSerNum_2->value(),
        ui->sbxOrder->value(),
        ui->dteOrder->date() });
}
