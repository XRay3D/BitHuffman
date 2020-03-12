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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    Regul::load(ui->cbxRegul);

    connect(ui->actionAddDep, &QAction::triggered, [this] { AddDepartment(this).exec(); });
    connect(ui->actionAddReg, &QAction::triggered, [this] { AddAdjuster(this).exec(); });

    if (1) {
        //        /* Первым делом необходимо создать объект, который будет использоваться для работы с данными нашей БД
        //         * и инициализировать подключение к базе данных
        //         * */
        db = new DataBase();
        if (!db->connectToDataBase()) {
            exit(0);
        }

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
        //        ui->tableViewSql->setItemDelegate(new BookDelegate(ui->tableViewSql));
        ui->tableViewSql->setColumnHidden(model->fieldIndex("id"), true);
        ui->tableViewSql->setSelectionMode(QAbstractItemView::SingleSelection);

        // Initialize the combo box:
        ui->cbxRegul->setModel(model->relationModel(adjIdx));
        ui->cbxRegul->setModelColumn(model->relationModel(adjIdx)->fieldIndex(TABLE_ADJ_NAME));
        //        ui.genreEdit->setModel(model->relationModel(genreIdx));
        //        ui.genreEdit->setModelColumn(model->relationModel(genreIdx)->fieldIndex("name"));

        // Lock and prohibit resizing of the width of the rating column:
        //        ui->tableViewSql->horizontalHeader()->setSectionResizeMode(model->fieldIndex("rating"), QHeaderView::ResizeToContents);
        ui->tableViewSql->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        ui->tableViewSql->horizontalHeader()->setSectionResizeMode(model->fieldIndex(TABLE_SN_ORDER_DATE), QHeaderView::Stretch);

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

        //        /* После чего производим наполнение таблицы базы данных
        //         * контентом, который будет отображаться в TableView
        //         * */
        //        //        for (int i = 0; i < 4; i++) {
        //        //            QVariantList data;
        //        //            int random = qrand(); // Получаем случайные целые числа для вставки а базу данных
        //        //            data.append(QDate::currentDate()); // Получаем текущую дату для вставки в БД
        //        //            data.append(QTime::currentTime()); // Получаем текущее время для вставки в БД
        //        //            // Подготавливаем полученное случайное число для вставки в БД
        //        //            data.append(random);
        //        //            // Подготавливаем сообщение для вставки в базу данных
        //        //            data.append("Получено сообщение от " + QString::number(random));
        //        //            // Вставляем данные в БД
        //        //            db->inserIntoTable(data);
        //        //        }

        //        /* Инициализируем модель для представления данных с заданием названий колонок */
        //        //        setupModel(TABLE, { tr("id"), tr("Дата"), tr("Время"), tr("Рандомное число"), tr("Сообщение") });
        //        setupModel(TABLE_SN, { "id", "Регулировщик", "Дата", "№ в мес.", "Серийные № (код)", "Заказ", "Дата Заказа" });

        //        /* Инициализируем внешний вид таблицы с данными */
        //        ui->tableViewSql->setModel(model); // Устанавливаем модель на TableView
        //        ui->tableViewSql->setColumnHidden(0, true); // Скрываем колонку с id записей
        //        // Разрешаем выделение строк
        //        //        ui->tableViewSql->setSelectionBehavior(QAbstractItemView::SelectRows);
        //        // Устанавливаем режим выделения лишь одно строки в таблице
        //        ui->tableViewSql->setSelectionMode(QAbstractItemView::SingleSelection);
        //        // Устанавливаем размер колонок по содержимому
        //        ui->tableViewSql->resizeColumnsToContents();
        //        //        ui->tableViewSql->setEditTriggers(QAbstractItemView::NoEditTriggers);
        //        ui->tableViewSql->horizontalHeader()->setStretchLastSection(true);
        //        model->select(); // Делаем выборку данных из таблицы

        //        //        QDataWidgetMapper* mapper = new QDataWidgetMapper(this);
        //        //        mapper->setModel(model);
        //        //        //        mapper->setItemDelegate(new BookDelegate(this));
        //        //        mapper->addMapping(new QLineEdit(this), model->fieldIndex(TABLE_MESSAGE));
        //        //        //        mapper->addMapping(ui.yearEdit, model->fieldIndex("year"));
        //        //        //        mapper->addMapping(ui.authorEdit, authorIdx);
        //        //        //        mapper->addMapping(ui.genreEdit, genreIdx);
        //        //        //        mapper->addMapping(ui.ratingEdit, model->fieldIndex("rating"));
        //        //        connect(ui->tableViewSql->selectionModel(), &QItemSelectionModel::currentRowChanged, mapper, &QDataWidgetMapper::setCurrentModelIndex);
    }

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
