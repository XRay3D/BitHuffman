#include "mainwindow.h"
#include "model.h"
#include "ui_mainwindow.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPageLayout>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    Regul::load(ui->cbxRegul);

    ui->dateEdit->setDate(QDate::currentDate());

    connect(ui->sbxSerNumCoded, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::fromSerNum);

    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &MainWindow::toSerNum);
    connect(ui->cbxRegul, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::toSerNum);
    connect(ui->sbxSerNum, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::toSerNum);

    ui->tableView->setModel(new Model(ui->tableView));
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    ui->tableView->installEventFilter(this);

    connect(ui->tableView, &QTableView::customContextMenuRequested, [this](const QPoint& pos) {
        QMenu menu;
        menu.addAction("Печать", [this] { printDialog(); });
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
    ui->sbxSerNum->setValue(Model::getLastSerNum(Regul::fromIndex(ui->cbxRegul->currentIndex()).id, ui->dateEdit->date()));
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
    if (QMessageBox::information(this, "A}{Tu|/|G", "Это действие нельзя отменить!!!", QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
        return;

    toSerNum();
    Model::addRecord({ //
        Regul::fromIndex(ui->cbxRegul->currentIndex()),
        ui->dateEdit->date(),
        ui->sbxSerNum->value(),
        ui->sbxSerNumCount->value(),
        ui->sbxOrder->value(),
        ui->dteOrder->date() });

    ui->sbxSerNum->setValue(Model::getLastSerNum(Regul::fromIndex(ui->cbxRegul->currentIndex()).id, ui->dateEdit->date()));
    ui->tableView->selectRow(ui->tableView->model()->rowCount() - 1);
    QTimer ::singleShot(100, [this] { printDialog(); });
}

void MainWindow::printDialog()
{
    QString text;
    for (auto& index : ui->tableView->selectionModel()->selectedRows()) {
        text += index.data().toString() + '\r' + Model::getRecord(index.row()).toString().replace('\n', ", ") + '\r';
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true); // must be before setmargin
    printer.setPageSize(QPrinter::A4);
    printer.setMargins({ 1, 1, 1, 1 });
    printer.setPageMargins({ 10, 10, 10, 10 }, QPageLayout::Millimeter);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, [text](QPrinter* pPrinter) {
        QTextEdit document;
        QFont f;
        f.setPointSizeF(14);
        document.setFont(f);
        document.setPlainText(text);
        document.setContentsMargins({ 1, 1, 1, 1 });
        document.print(pPrinter);
    });
    preview.exec();
}
