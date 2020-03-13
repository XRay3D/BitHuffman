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

class Delegate : public QSqlRelationalDelegate {
public:
    Delegate(QObject* parent)
        : QSqlRelationalDelegate(parent)
    {
    }

    //    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    //    {
    //        //        switch (index.column()) {
    //        //        case 0:
    //        //            [[fallthrough]];
    //        //        case 1: {
    //        //            QSpinBox* sb = new QSpinBox(parent);
    //        //            sb->setFrame(false);
    //        //            sb->setMaximum(index.data().toInt());
    //        //            sb->setMinimum(index.data().toInt());
    //        //            sb->setButtonSymbols(QSpinBox::NoButtons);
    //        //            return sb;
    //        //        }
    //        //        case 3: {
    //        //            QLineEdit* le = new QLineEdit(parent);
    //        //            le->setFrame(false);
    //        //            le->setText(index.data().toString());
    //        //            return le;
    //        //        }
    //        //        }
    //        return QSqlRelationalDelegate::createEditor(parent, option, index);
    //    }

    // QAbstractItemDelegate interface
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
            QSqlRelationalDelegate::paint(painter, opt, index);
        }
    }
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    Regul::load(ui->cbxRegul);

    connect(ui->actionAddDep, &QAction::triggered, [this] { AddDepartment(this).exec(); });
    connect(ui->actionAddReg, &QAction::triggered, [this] { AddAdjuster(this).exec(); });

    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateOrder->setDate(QDate::currentDate());

    if (1) {
        connect(ui->sbxSerNumCoded, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::fromEncSerNum);

        connect(ui->dateEdit, &QDateEdit::dateChanged, this, &MainWindow::toEncSerNum);
        connect(ui->cbxRegul, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::toEncSerNum);
        connect(ui->sbxSerNum, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::toEncSerNum);

        connect(ui->sbxOrder, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::warningOrder);
        connect(ui->dateOrder, &QDateEdit::dateChanged, this, &MainWindow::warningOrder);

        //    ui->tableView->setModel(new Model(ui->tableView));
        if (ui->tableView->model()) {
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
        }
    }

    toEncSerNum();

    QSettings settings("GenSerNum.ini", QSettings::IniFormat);
    ui->groupBoxFormat->setChecked(settings.value("groupBoxFormat", false).toBool());
    ui->groupBoxFormatBit->setChecked(settings.value("groupBoxFormatBit", false).toBool());
    restoreState(settings.value("state").toByteArray());
    restoreGeometry(settings.value("geometry").toByteArray());

    {
        /* Первым делом необходимо создать объект, который
     * будет использоваться для работы с данными нашей БД
     * и инициализировать подключение к базе данных
     */
        db = new DataBase();
        if (!db->connectToDataBase())
            exit(0);

        //return;
        // Create the data model:
        model = new QSqlRelationalTableModel(ui->tableViewSql);
        model->setEditStrategy(QSqlTableModel::OnManualSubmit);
        model->setTable(TABLE_SN);

        // Remember the indexes of the columns:
        adjIdx = model->fieldIndex(TABLE_SN_ADJ_ID);
        ordNumIdx = model->fieldIndex(TABLE_SN_ORDER_NUM);
        ordDateIdx = model->fieldIndex(TABLE_SN_ORDER_DATE);

        // Set the relations to the other database tables:
        model->setRelation(adjIdx, QSqlRelation(TABLE_ADJ, TABLE_ADJ_ID, TABLE_ADJ_NAME));
        model->setRelation(ordNumIdx, QSqlRelation(TABLE_ORDER, "id", TABLE_ORD_NUM));
        model->setRelation(ordDateIdx, QSqlRelation(TABLE_ORDER, "id", TABLE_ORD_DATE));

        // Set the localized header captions:

        model->setHeaderData(model->fieldIndex(TABLE_SN_ADJ_ID), Qt::Horizontal, "Регулировщик");
        model->setHeaderData(model->fieldIndex(TABLE_SN_DATE_CREATION), Qt::Horizontal, "Дата изг.");
        model->setHeaderData(model->fieldIndex(TABLE_SN_MONTH_COUNT), Qt::Horizontal, "№ в мес.");
        model->setHeaderData(model->fieldIndex(TABLE_SN_CODED), Qt::Horizontal, "Серийные № (код)");
        model->setHeaderData(model->fieldIndex(TABLE_SN_ORDER_NUM), Qt::Horizontal, "№ Заказа");
        model->setHeaderData(model->fieldIndex(TABLE_SN_ORDER_DATE), Qt::Horizontal, "Дата Заказа");

        // Populate the model:
        if (!model->select()) {
            showError(model->lastError());
            return;
        }

        // Set the model and hide the ID column:
        ui->tableViewSql->setModel(model);
        ui->tableViewSql->setItemDelegate(new Delegate(ui->tableViewSql));
        ui->tableViewSql->setColumnHidden(model->fieldIndex("id"), true);
        ui->tableViewSql->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->tableViewSql->setSelectionBehavior(QAbstractItemView::SelectRows);

        // Initialize the combo box:
        ui->cbxRegul->setModel(model->relationModel(adjIdx));
        ui->cbxRegul->setModelColumn(model->relationModel(adjIdx)->fieldIndex(TABLE_ADJ_NAME));

        // Lock and prohibit resizing of the width of the rating column:
        ui->tableViewSql->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ui->tableViewSql->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        ui->tableViewSql->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        ui->tableViewSql->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
        ui->tableViewSql->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);

        //        ui->tableViewSql->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
        //        QDataWidgetMapper* mapper = new QDataWidgetMapper(this);
        //        mapper->setModel(model);
        //        mapper->setItemDelegate(new BookDelegate(this));
        //        mapper->addMapping(ui.titleEdit, model->fieldIndex("title"));
        //        mapper->addMapping(ui.yearEdit, model->fieldIndex("year"));
        //        mapper->addMapping(ui.authorEdit, authorIdx);
        //        mapper->addMapping(ui.genreEdit, genreIdx);
        //        mapper->addMapping(ui.ratingEdit, model->fieldIndex("rating"));
        //        connect(ui->tableViewSql->selectionModel(), &QItemSelectionModel::currentRowChanged, mapper, &QDataWidgetMapper::setCurrentModelIndex);

        ui->tableViewSql->setCurrentIndex(model->index(0, 0));
    }
    //    {
    //        auto w = new QTableView;
    //        auto model = new QSqlTableModel(w);
    //        model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    //        model->setTable(TABLE_ORDER);
    //        w->setModel(model);
    //        w->setSortingEnabled(true);
    //        w->show();
    //        model->submitAll();
    //    }
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
    if constexpr (0) {
        ui->dateEdit->setDate(date);
        ui->cbxRegul->setCurrentIndex(Regul::toIndex(regId));
        ui->sbxSerNum->setValue(serNum);
        if (auto index = Model::getIndex(ui->sbxSerNumCoded->value()); index > -1) {
            ui->tableView->selectRow(index);
            ui->tableView->showRow(index);
        } else {
            ui->tableView->selectionModel()->clear();
        }
    } else {
        ui->dateEdit->setDate(date);
        ui->cbxRegul->setCurrentIndex(Regul::toIndex(regId));
        ui->sbxSerNum->setValue(serNum);

        {
            QSqlQuery q;

            //            q.setForwardOnly(true);
            q.prepare("SELECT * FROM " + TABLE_SN + " WHERE " + TABLE_SN_CODED + " = :ref_id1");
            q.bindValue(":ref_id1", ui->sbxSerNumCoded->value());

            if (!q.exec())
                qDebug() << "SQL QUERY ERROR:" << q.lastError().text();
            if (q.next()) {
                {
                    auto tv = new QTableView;
                    auto m = new QSqlQueryModel(tv);
                    m->setQuery(q);
                    if (m->lastError().isValid())
                        qDebug() << m->lastError();
                    tv->setModel(m);
                    tv->show();
                }

                //                int index;
                //                qDebug() << (index = q.value(0).toInt()); // << q.value(1) << q.value(2).toDate().toString("dd.MM.yyyy");
                //                QModelIndexList mdl = model->match(model->index(0, 4), Qt::DisplayRole, ui->sbxSerNumCoded->value(), 1);
                //                qDebug() << mdl;
                //                //auto m = model->relationModel(depIdx);
                //                //return m->data(m->index(ui->comboBoxDepartment->currentIndex(), m->fieldIndex(TABLE_DEP_NUMBER))).toInt();
                //                ui->tableViewSql->selectRow(mdl.first().row());
                //                ui->tableViewSql->showRow(mdl.first().row());
            } else {
                ui->tableViewSql->selectionModel()->clear();
            }
        }
    }

    tos();

    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &MainWindow::toEncSerNum);
    connect(ui->cbxRegul, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::toEncSerNum);
    connect(ui->sbxSerNum, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::toEncSerNum);
}

void MainWindow::on_pbGen_clicked()
{
    if (!ui->tableView->model())
        return;
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

void MainWindow::setupModel(const QString& tableName, const QStringList& headers)
{
    model = new QSqlRelationalTableModel(ui->tableViewSql);
    //model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setTable(tableName);

    /* Устанавливаем названия колонок в таблице с сортировкой данных
     * */
    for (int i = 0; i < model->columnCount(); i++) {
        model->setHeaderData(i, Qt::Horizontal, headers.value(i));
    }
    // Устанавливаем сортировку по возрастанию данных по нулевой колонке
    model->setSort(0, Qt::AscendingOrder);
}
