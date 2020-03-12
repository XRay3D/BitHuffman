#include "mainwindow.h"
#include "model.h"
#include "ui_mainwindow.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPageLayout>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QSettings>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>

class ItemDelegate : public QStyledItemDelegate {
    //Q_OBJECT
public:
    ItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }
    virtual ~ItemDelegate() override {}

    // QAbstractItemDelegate interface
public:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        if (option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
            QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Normal /*Disabled*/;
            //            if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
            //                cg = QPalette::Inactive;
            painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
        } else {
            QVariant value = index.data(Qt::BackgroundRole);
            if (value.canConvert<QBrush>()) {
                QPointF oldBO = painter->brushOrigin();
                painter->setBrushOrigin(option.rect.topLeft());
                painter->fillRect(option.rect, qvariant_cast<QBrush>(value));
                painter->setBrushOrigin(oldBO);
            }
        }
        {
            auto opt = option;
            opt.state &= ~QStyle::State_Selected;
            opt.state &= ~QStyle::State_HasFocus;
            QStyledItemDelegate::paint(painter, opt, index);
        }
    }
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    Regul::load(ui->cbxRegul, ui->comboBox);
    ui->comboBox->lineEdit()->setReadOnly(true);
    ui->comboBox->lineEdit()->setAlignment(Qt::AlignCenter);

    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateOrder->setDate(QDate::currentDate());

    connect(ui->sbxSerNumCoded, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::fromEncSerNum);

    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &MainWindow::toEncSerNum);
    connect(ui->cbxRegul, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::toEncSerNum);
    connect(ui->sbxSerNum, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::toEncSerNum);

    connect(ui->sbxOrder, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::warningOrder);
    connect(ui->dateOrder, &QDateEdit::dateChanged, this, &MainWindow::warningOrder);

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

    ui->tableView->setItemDelegate(new ItemDelegate(ui->tableView));

    toEncSerNum();

    QSettings settings("GenSerNum.ini", QSettings::IniFormat);
    ui->groupBoxFormat->setChecked(settings.value("groupBoxFormat", false).toBool());
    ui->groupBoxFormatBit->setChecked(settings.value("groupBoxFormatBit", false).toBool());
    restoreState(settings.value("state").toByteArray());
    restoreGeometry(settings.value("geometry").toByteArray());
}

MainWindow::~MainWindow()
{
    QSettings settings("GenSerNum.ini", QSettings::IniFormat);
    settings.setValue("groupBoxFormat", ui->groupBoxFormat->isChecked());
    settings.setValue("groupBoxFormatBit", ui->groupBoxFormatBit->isChecked());
    settings.setValue("state", saveState());
    settings.setValue("geometry", saveGeometry());
    delete ui;
}

void MainWindow::toEncSerNum()
{
    disconnect(ui->sbxSerNumCoded, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::fromEncSerNum);
    ui->sbxSerNum->setValue(Model::getLastSerNum(Regul::fromIndex(ui->cbxRegul->currentIndex()).id, ui->dateEdit->date()));
    ui->sbxSerNumCoded->setValue(
        Record::toSerNum(Regul::fromIndex(ui->cbxRegul->currentIndex()).id,
            ui->dateEdit->date(),
            ui->sbxSerNum->value()));

    tos();

    connect(ui->sbxSerNumCoded, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::fromEncSerNum);
}

void MainWindow::fromEncSerNum()
{
    disconnect(ui->dateEdit, &QDateEdit::dateChanged, this, &MainWindow::toEncSerNum);
    disconnect(ui->cbxRegul, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::toEncSerNum);
    disconnect(ui->sbxSerNum, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::toEncSerNum);

    auto [regId, date, serNum] = Record::fromSerNum(ui->sbxSerNumCoded->value());
    ui->dateEdit->setDate(date);
    ui->cbxRegul->setCurrentIndex(Regul::toIndex(regId));
    ui->sbxSerNum->setValue(serNum);
    if (auto index = Model::getIndex(ui->sbxSerNumCoded->value()); index > -1) {
        ui->tableView->selectRow(index);
        ui->tableView->showRow(index);
    } else {
        ui->tableView->selectionModel()->clear();
    }

    tos();

    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &MainWindow::toEncSerNum);
    connect(ui->cbxRegul, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::toEncSerNum);
    connect(ui->sbxSerNum, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::toEncSerNum);
}

void MainWindow::on_pbGen_clicked()
{
    if (warningOrder() || QMessageBox::information(this, "A}{Tu|/|G", "Это действие нельзя отменить!!!", QMessageBox::Cancel, QMessageBox::Apply) == QMessageBox::Cancel)
        return;

    toEncSerNum();
    Model::addRecord({ //
        Regul::fromIndex(ui->cbxRegul->currentIndex()),
        ui->dateEdit->date(),
        ui->sbxSerNum->value(),
        ui->sbxSerNumCount->value(),
        ui->sbxOrder->value(),
        ui->dateOrder->date() });

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

void MainWindow::tos()
{
    QString sqatres1(QString::number(Regul::fromIndex(ui->cbxRegul->currentIndex()).id));

    ui->spinBox_1->setValue(sqatres1.mid(0, 1).toInt());
    ui->spinBox_2->setValue(Regul::fromIndex(ui->cbxRegul->currentIndex()).id);
    ui->comboBox->setCurrentIndex(ui->cbxRegul->currentIndex());

    ui->spinBox_4->setValue(ui->dateEdit->date().month());
    ui->spinBox_6->setValue(ui->dateEdit->date().year() - 2000);
    ui->spinBox_8->setValue(ui->sbxSerNum->value());

    int esn = ui->sbxSerNumCoded->value();

    for (auto sbx : {
             ui->sbxBin_1,
             ui->sbxBin_2,
             ui->sbxBin_3,
             ui->sbxBin_4,
             ui->sbxBin_5,
             ui->sbxBin_6,
             ui->sbxBin_7,
             ui->sbxBin_8,
             ui->sbxBin_9,
             ui->sbxBin_10,
             ui->sbxBin_11,
             ui->sbxBin_12,
             ui->sbxBin_13,
             ui->sbxBin_14,
             ui->sbxBin_15,
             ui->sbxBin_16,
             ui->sbxBin_17,
             ui->sbxBin_18,
             ui->sbxBin_19,
             ui->sbxBin_20,
             ui->sbxBin_21,
             ui->sbxBin_22,
             ui->sbxBin_23,
         }) {
        sbx->setValue(esn & 0x1);
        esn >>= 1;
    }
}

bool MainWindow::warningOrder()
{
    if (auto row = Model::getOrderRow(ui->sbxOrder->value(), ui->dateOrder->date()); row > -1) {
        ui->tableView->selectRow(row);
        return (QMessageBox::warning(this, "A}{Tu|/|G", "Данный № заказа уже есть в базе!!!\n\n"
                                                        "Если хотите добавить к заказу приборы,\n"
                                                        "то необходимо указать разницу в количестве изделий,\n"
                                                        "иначе ни чего не делать.",
                    QMessageBox::Cancel, QMessageBox::Yes)
            == QMessageBox::Cancel);
    }
    return false;
}

void MainWindow::on_groupBoxFormat_toggled(bool arg1)
{
    //qDebug() << arg1 << ui->groupBoxFormat->children();
    for (auto obj : ui->groupBoxFormat->children()) {
        if (auto w = qobject_cast<QWidget*>(obj); w)
            w->setVisible(arg1);
    }
    ui->gridLayoutFormat->setMargin(arg1 ? 6 : 3);
}

void MainWindow::on_groupBoxFormatBit_toggled(bool arg1)
{
    //qDebug() << arg1 << ui->groupBoxFormatBit->children();
    for (auto obj : ui->groupBoxFormatBit->children()) {
        if (auto w = qobject_cast<QWidget*>(obj); w)
            w->setVisible(arg1);
    }
    ui->gridLayoutFormatBit->setMargin(arg1 ? 6 : 0);
}

void MainWindow::on_pushButton_test_clicked()
{
    auto rnd = [](int a, int b) { return rand() % (b - a + 1) + a; };
    for (int i = 0; i < 10000; ++i) {
        qDebug() << i << 10000;
        Model::addRecord({
            //
            Regul::fromIndex(rnd(0, 6)),
            QDate(rnd(2020, 2029), rnd(1, 12),rnd(1, 28)), //ui->dateEdit->date(),
            rnd(1, 100), //ui->sbxSerNum->value(),
            rnd(1, 100), //ui->sbxSerNumCount->value(),
            rnd(1, 100000), //ui->sbxOrder->value(),
            QDate(rnd(2020, 2029), rnd(1, 12),rnd(1, 28)), //ui->dateOrder->date() });
        });
    }
}
