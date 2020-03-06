#include "mainwindow.h"
#include "model.h"
#include "ui_mainwindow.h"

#include <QClipboard>
#include <QContextMenuEvent>
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

    Regul::load(ui->cbxRegul);

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
    QString sqatres2(ui->dateEdit->date().toString("MMyy"));
    QString sqatres3(QString::number(ui->sbxSerNum->value()));

    int i = 0;
    ui->spinBox_1->setValue(sqatres1.mid(i++, 1).toInt());
    ui->spinBox_3->setValue(sqatres1.mid(i++, 1).toInt());
    i = 0;
    ui->spinBox_4->setValue(sqatres2.mid(i++, 1).toInt());
    ui->spinBox_5->setValue(sqatres2.mid(i++, 1).toInt());
    ui->spinBox_6->setValue(sqatres2.mid(i++, 1).toInt());
    ui->spinBox_7->setValue(sqatres2.mid(i++, 1).toInt());

    i = sqatres3.length();

    //QVector sbxv{ ui->spinBox_11, ui->spinBox_10, ui->spinBox_9, ui->spinBox_8 };
    for (auto sbx : { ui->spinBox_11, ui->spinBox_10, ui->spinBox_9, ui->spinBox_8 }) {
        if (i)
            sbx->setValue(sqatres3.mid(--i, 1).toInt());
        else
            sbx->setValue(0);
    }

    int esn = ui->sbxSerNumCoded->value();

    for (auto chbx : {
             ui->checkBox_1,
             ui->checkBox_2,
             ui->checkBox_3,
             ui->checkBox_4,
             ui->checkBox_5,
             ui->checkBox_6,
             ui->checkBox_7,
             ui->checkBox_8,
             ui->checkBox_9,
             ui->checkBox_10,
             ui->checkBox_11,
             ui->checkBox_12,
             ui->checkBox_13,
             ui->checkBox_14,
             ui->checkBox_15,
             ui->checkBox_16,
             ui->checkBox_17,
             ui->checkBox_18,
             ui->checkBox_19,
             ui->checkBox_20,
             ui->checkBox_21,
             ui->checkBox_22,
             ui->checkBox_23,
         }) {
        chbx->setChecked(esn & 0x1);
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
