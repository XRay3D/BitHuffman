#include "mainwindow.h"
#include "database/addadjuster.h"
#include "database/adddepartment.h"
#include "database/database.h"
#include "model.h"
#include "ui_mainwindow.h"
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDataWidgetMapper>
#include <QLineEdit>
#include <QMenu>
#include <QPageLayout>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QSettings>
#include <QStyle>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#include <database/addsernums.h>

class MySqlRelationalTableModel : public QSqlRelationalTableModel {
    //Q_OBJECT

public:
    MySqlRelationalTableModel(QObject* parent = nullptr)
        : QSqlRelationalTableModel(parent)
    {
    }
    virtual ~MySqlRelationalTableModel() override { }

    // QAbstractItemModel interface
public:
    QVariant data(const QModelIndex& index, int role) const override
    {
        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case 2:
            case 4:
                return QSqlRelationalTableModel::data(index, role).toDate().toString("dd.MM.yyyy");
            }
        }
        return QSqlRelationalTableModel::data(index, role);
    }
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actionAddDep, &QAction::triggered, [this] { AddDepartment(this).exec(); });
    connect(ui->actionAddReg, &QAction::triggered, [this] { AddAdjuster(this).exec(); });
    connect(ui->actionOrders, &QAction::triggered, [this] { showOrders(); });
    connect(ui->actionPrint, &QAction::triggered, [this] { printDialog(); });
    ui->actionPrint->setShortcut(QKeySequence::Print);

    QSettings settings("GenSerNum.ini", QSettings::IniFormat);
    ui->groupBoxFormat->setChecked(settings.value("groupBoxFormat", false).toBool());
    ui->groupBoxFormatBit->setChecked(settings.value("groupBoxFormatBit", false).toBool());
    restoreState(settings.value("state").toByteArray());
    restoreGeometry(settings.value("geometry").toByteArray());

    // Первым делом необходимо создать объект, который будет использоваться для работы с данными нашей БД
    // и инициализировать подключение к базе данных
    db = new DataBase(this);
    if (!db->connectToDataBase())
        exit(0);

    {
        modelSerNum = new QSqlTableModel(ui->tvSerNums);
        modelSerNum->setEditStrategy(QSqlTableModel::OnManualSubmit);
        modelSerNum->setTable(TABLE_ENC_SER_NUM /*VIEW_SN*/);
        ui->tvSerNums->setModel(modelSerNum);
        if (!modelSerNum->select()) {
            showError(modelSerNum->lastError());
            exit(0);
        }

        // Set the model and hide the ID column:
        //  ui->tvOrders->setItemDelegate(new Delegate(ui->tvOrders));
        ui->tvSerNums->setModel(modelSerNum);
        ui->tvSerNums->setColumnHidden(modelSerNum->fieldIndex(TESN_ORDER), true);
        ui->tvSerNums->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tvSerNums->setSelectionMode(QAbstractItemView::SingleSelection);

        // Lock and prohibit resizing of the width of the rating column:
        //  ui->tvSerNums->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        ui->tvSerNums->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ui->tvSerNums->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        ui->tvSerNums->verticalHeader()->setDefaultSectionSize(QFontMetrics(QFont()).height());
        //        ui->tvSerNums->verticalHeader()->setVisible(false);

        // Set the localized header captions:
        modelSerNum->setHeaderData(modelSerNum->fieldIndex(TESN_ESN_KEY), Qt::Horizontal, "Сер. №");
        modelSerNum->setHeaderData(modelSerNum->fieldIndex(TESN_MONTH_COUNTER), Qt::Horizontal, "Мес. №");
        modelSerNum->setHeaderData(modelSerNum->fieldIndex(TESN_ORDER), Qt::Horizontal, "№ заказа");
        connect(ui->tvSerNums->selectionModel(), &QItemSelectionModel::currentRowChanged,
            [this](const QModelIndex& current, const QModelIndex& /*previous*/) {
                ui->sbxSerNumCoded->setValue(current.siblingAtColumn(0).data().toInt());
            });
    }

    {
        // Create the data model:
        modelOrder = new MySqlRelationalTableModel /*QSqlRelationalTableModel*/ (ui->tvOrders);
        modelOrder->setEditStrategy(QSqlTableModel::OnManualSubmit);
        modelOrder->setTable(TABLE_ORDER /*VIEW_SN*/);

        // Remember the indexes of the columns:
        const int adjIdx = modelOrder->fieldIndex(TORD_ADJ);
        //    const int ordDateIdx = model->fieldIndex(TABLE_SN_ORDER_DATE);
        //    const int ordNumIdx = model->fieldIndex(TABLE_SN_ORDER_NUM);

        // Set the relations to the other database tables:
        modelOrder->setRelation(adjIdx, QSqlRelation(TABLE_ADJ, TADJ_ID_KEY, TADJ_NAME));
        //    model->setRelation(ordDateIdx, QSqlRelation(TABLE_ORDER, "id", TABLE_ORD_DATE));
        //    model->setRelation(ordNumIdx, QSqlRelation(TABLE_ORDER, "id", TABLE_ORD_NUM));

        // Set the localized header captions:
        modelOrder->setHeaderData(modelOrder->fieldIndex(TORD_ADJ), Qt::Horizontal, "Исполнитель");
        modelOrder->setHeaderData(modelOrder->fieldIndex(TORD_COUNT), Qt::Horizontal, "Кол-во.");
        modelOrder->setHeaderData(modelOrder->fieldIndex(TORD_DATE_ORDER), Qt::Horizontal, "Дата заказа");
        modelOrder->setHeaderData(modelOrder->fieldIndex(TORD_DATE_CREATION), Qt::Horizontal, "Дата изг.");
        modelOrder->setHeaderData(modelOrder->fieldIndex(TORD_NUM), Qt::Horizontal, "№ заказа");

        // Populate the model:
        if (!modelOrder->select()) {
            showError(modelOrder->lastError());
            exit(0);
        }

        // Set the model and hide the ID column:
        ui->tvOrders->setModel(modelOrder);
        ui->tvOrders->setColumnHidden(modelOrder->fieldIndex(TORD_KEY), true);
        ui->tvOrders->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tvOrders->setSelectionMode(QAbstractItemView::SingleSelection);

        // Lock and prohibit resizing of the width of the rating column:
        ui->tvOrders->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        ui->tvOrders->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        ui->tvOrders->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        ui->tvOrders->verticalHeader()->setDefaultSectionSize(QFontMetrics(QFont()).height());

        connect(ui->tvOrders->selectionModel(), &QItemSelectionModel::currentRowChanged,
            [this](const QModelIndex& current, const QModelIndex& /*previous*/) {
                modelSerNum->setFilter(QString("%1='%2'").arg(TESN_ORDER).arg(current.siblingAtColumn(0).data().toInt()));
                ui->sbxSerNumCoded->setValue(modelSerNum->data(modelSerNum->index(0, 0)).toInt());
            });
    }

    modelSerNum->setFilter(QString("%1='%2'").arg(TESN_ORDER).arg(-1));
    //ui->tvOrders->selectRow(0);

    if (0) {
        //        QDataWidgetMapper* mapper = new QDataWidgetMapper(this);
        //        mapper->setModel(model);
        //        mapper->setItemDelegate(new BookDelegate(this));
        //        mapper->addMapping(ui.titleEdit, model->fieldIndex("title"));
        //        mapper->addMapping(ui.yearEdit, model->fieldIndex("year"));
        //        mapper->addMapping(ui.authorEdit, authorIdx);
        //        mapper->addMapping(ui.genreEdit, genreIdx);
        //        mapper->addMapping(ui.ratingEdit, model->fieldIndex("rating"));
        //        connect(ui->tvOrders->selectionModel(), &QItemSelectionModel::currentRowChanged, mapper, &QDataWidgetMapper::setCurrentModelIndex);
    }
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

void MainWindow::on_pbGen_clicked()
{
    AddSerNums d(this);
    connect(&d, &AddSerNums::setOrderFilter, this, &MainWindow::setOrderFilter);
    if (d.exec()) {
        //        modelOrder->select();
        //        const auto [min, max] = d.getRange();
        //        modelOrder->setFilter(QString("%1 BETWEEN '%2' AND '%3'")
        //                                  .arg(VSN_CODED)
        //                                  .arg(min)
        //                                  .arg(max));
        printDialog();
    }
}

void MainWindow::printDialog()
{
    QString text;

    auto indexes { ui->tvOrders->selectionModel()->selectedRows() };
    if (indexes.isEmpty()) {
        QMessageBox::information(this, "", "Нет выделенной строки заказа для печати.");
        return;
    }

    text += modelOrder->data(modelOrder->index(indexes.first().row(), 1)).toString() + '\n';
    while (modelSerNum->canFetchMore())
        modelSerNum->fetchMore();

    for (int row = 0; row < modelSerNum->rowCount(); ++row) {
        if (row)
            text += ", ";
        text += modelSerNum->data(modelSerNum->index(row, 0)).toString();
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
        f.setPointSizeF(13);
        document.setFont(f);
        document.setPlainText(text);
        document.print(pPrinter);
    });
    preview.exec();
}

void MainWindow::tos()
{
    //qDebug() << Q_FUNC_INFO;

    int esn = ui->sbxSerNumCoded->value();

    {
        auto [adjId, date, numInMonth] = Record::fromSerNum(esn);

        const QString sqatres1(QString::number(adjId));
        const QString sqatres2(date.toString("MMyy"));
        const QString sqatres3(QString::number(numInMonth));

        ui->sbxDepNum->setValue(sqatres1.midRef(0, 1).toInt());
        ui->sbxAdjNum->setValue(adjId);
        ui->sbxMonth->setValue(sqatres2.midRef(0, 2).toInt());
        ui->sbxYear->setValue(sqatres2.midRef(2, 2).toInt());
        ui->sbxCtyInMonth->setValue(numInMonth);
    }

    for (auto chbx : {
             ui->chbxBit1,
             ui->chbxBit2,
             ui->chbxBit3,
             ui->chbxBit4,
             ui->chbxBit5,
             ui->chbxBit6,
             ui->chbxBit7,
             ui->chbxBit8,
             ui->chbxBit9,
             ui->chbxBit10,
             ui->chbxBit11,
             ui->chbxBit12,
             ui->chbxBit13,
             ui->chbxBit14,
             ui->chbxBit15,
             ui->chbxBit16,
             ui->chbxBit17,
             ui->chbxBit18,
             ui->chbxBit19,
             ui->chbxBit20,
             ui->chbxBit21,
             ui->chbxBit22,
             ui->chbxBit23,
         }) {
        chbx->setTristate(true);
        chbx->setCheckState((esn & 0x1) ? Qt::PartiallyChecked : Qt::Unchecked);
        chbx->setCheckable(false);
        esn >>= 1;
    }
}

void MainWindow::on_groupBoxFormat_toggled(bool arg1)
{
    for (auto obj : ui->groupBoxFormat->children()) {
        if (auto w = qobject_cast<QWidget*>(obj); w)
            w->setVisible(arg1);
    }
    ui->gridLayoutFormat->setMargin(arg1 ? 6 : 3);
}

void MainWindow::on_groupBoxFormatBit_toggled(bool arg1)
{
    for (auto obj : ui->groupBoxFormatBit->children()) {
        if (auto w = qobject_cast<QWidget*>(obj); w)
            w->setVisible(arg1);
    }
    ui->gridLayoutFormatBit->setMargin(arg1 ? 6 : 0);
}

void MainWindow::on_sbxSerNumCoded_valueChanged(int arg1)
{
    auto idx = modelOrder->match(modelOrder->index(0, 4), Qt::DisplayRole, arg1, 1, Qt::MatchExactly);
    if (idx.isEmpty()) {

    } else {
        ui->tvOrders->selectRow(idx.first().row());
        ui->tvOrders->showRow(idx.first().row());
    }
    tos();
}

void MainWindow::on_pbReset_clicked()
{
    modelOrder->setFilter("");
    modelSerNum->setFilter(QString("%1='%2'").arg(TESN_ORDER).arg(-1));
}

void MainWindow::on_pbFind_clicked()
{
    modelSerNum->setFilter(QString("%1='%2'").arg(TESN_ESN_KEY).arg(ui->sbxSerNumCoded->value()));
    setOrderFilter(QString("%1='%2'").arg(TORD_KEY).arg(modelSerNum->record(0).value(TESN_ORDER).toString()));
    //modelOrder->setFilter(QString("%1='%2'").arg(VSN_CODED).arg(ui->sbxSerNumCoded->value()));
}

void MainWindow::on_tvOrders_doubleClicked(const QModelIndex& index)
{
    auto rec { modelOrder->record() };
    QString data { index.data(Qt::EditRole).toString() };
    const int column { index.column() };
    if (auto r { modelOrder->relation(column) }; r.isValid()) {
        QSqlTableModel m;
        m.setTable(modelOrder->tableName());
        modelOrder->setFilter(QString("%1 IN(SELECT %2 FROM %3 WHERE %4 = '%5')")
                                  .arg(m.record().fieldName(column))
                                  .arg(r.indexColumn())
                                  .arg(r.tableName())
                                  .arg(r.displayColumn())
                                  .arg(data));
    } else {
        modelOrder->setFilter(QString("%1='%2'").arg(rec.fieldName(column)).arg(data));
    }
}

void MainWindow::showOrders()
{
    QDialog d(this);
    auto layout = new QVBoxLayout(&d);
    layout->setObjectName(QString::fromUtf8("layout"));
    auto tv = new QTableView(&d);

    tv->setObjectName(QString::fromUtf8("tv"));
    tv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    tv->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tv->setDragDropOverwriteMode(false);
    tv->setAlternatingRowColors(true);
    tv->setSortingEnabled(true);

    layout->addWidget(tv);

    auto m = new QSqlTableModel(tv);

    auto pb = new QPushButton("Сброс", &d);
    layout->addWidget(pb);
    connect(pb, &QPushButton::clicked, [m] { m->setFilter(""); });

    m->setTable(TABLE_ENC_SER_NUM /*TABLE_ORDER*/);
    //    m->setHeaderData(m->fieldIndex(TABLE_ORD_CTY), Qt::Horizontal, "Кол-во в заказе");
    //    m->setHeaderData(m->fieldIndex(TABLE_ORD_DATE), Qt::Horizontal, "Дата Заказа");
    //    m->setHeaderData(m->fieldIndex(TABLE_ORD_NUM), Qt::Horizontal, "№ Заказа");
    m->select();

    tv->setModel(m);

    connect(tv, &QTableView::doubleClicked, [m](const QModelIndex& index) {
        auto r { m->record() };
        QString data { index.data().toString() };
        m->setFilter(QString("%1='%2'").arg(r.fieldName(index.column())).arg(data));
        m->select();
    });
    //    tv->setColumnHidden(0, true);
    tv->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tv->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    tv->verticalHeader()->setDefaultSectionSize(QFontMetrics(QFont()).height());

    d.restoreGeometry(this->saveGeometry());
    d.exec();
}

void MainWindow::setOrderFilter(const QString& filter)
{
    modelOrder->setFilter(filter);
    ui->tvOrders->selectionModel()->reset();
    ui->tvOrders->selectRow(0);
}
