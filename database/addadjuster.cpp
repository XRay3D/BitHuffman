#include "addadjuster.h"
#include "ui_addadjuster.h"

#include "database.h"
#include <QDataWidgetMapper>
#include <QLineEdit>
#include <QMessageBox>
#include <QtSql>

class Delegate : public QSqlRelationalDelegate {
public:
    Delegate(QObject* parent)
        : QSqlRelationalDelegate(parent)
    {
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        switch (index.column()) {
        case 0:
            [[fallthrough]];
        case 1: /* {
            QSpinBox* sb = new QSpinBox(parent);
            sb->setFrame(false);
            sb->setButtonSymbols(QSpinBox::NoButtons);
            sb->setReadOnly(true);
            return sb;
        }*/
        {
            QLineEdit* le = new QLineEdit(parent);
            le->setFrame(false);
            le->setText(index.data().toString());
            le->setReadOnly(true);
            return le;
        }
        case 3: {
            QLineEdit* le = new QLineEdit(parent);
            le->setFrame(false);
            le->setText(index.data().toString());
            return le;
        }
        }
        return QSqlRelationalDelegate::createEditor(parent, option, index);
    }
};

AddAdjuster::AddAdjuster(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AddAdjuster)
{
    ui->setupUi(this);

    // Create the data model:
    model = new QSqlRelationalTableModel(ui->tableView);

    model->setEditStrategy(QSqlTableModel::OnFieldChange);
    model->setTable(TABLE_ADJ);

    // Remember the indexes of the columns:
    depIdx = model->fieldIndex(TADJ_DEP);
    //    const auto genreIdx = model->fieldIndex("genre");

    // Set the relations to the other database tables:
    model->setRelation(depIdx, QSqlRelation(TABLE_DEPARTMENT, TDEP_NUMBER, TDEP_NAME));
    //    model->setRelation(genreIdx, QSqlRelation("genres", "id", "name"));

    // Set the header captions:
    model->setHeaderData(model->fieldIndex(TADJ_ID_KEY), Qt::Horizontal, "№ исполнителя");
    model->setHeaderData(model->fieldIndex(TADJ_DEP), Qt::Horizontal, "Подразделение");
    model->setHeaderData(model->fieldIndex(TADJ_NAME), Qt::Horizontal, "Имя");

    // Populate the model:
    if (!model->select()) {
        showError(model->lastError());
        reject();
        return;
    }

    // Set the model and hide the ID column:
    ui->tableView->setModel(model);
    ui->tableView->setItemDelegate(new Delegate(ui->tableView));
    ui->tableView->setColumnHidden(model->fieldIndex("id"), true);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Lock and prohibit resizing of the width of the rating column:
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(model->fieldIndex(TADJ_NAME), QHeaderView::Stretch);
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableView->verticalHeader()->setDefaultSectionSize(QFontMetrics(QFont()).height());
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->setEditTriggers(QAbstractItemView::DoubleClicked);

    // Initialize the Department combo box:
    ui->comboBoxDepartment->setModel(model->relationModel(depIdx));
    ui->comboBoxDepartment->setModelColumn(model->relationModel(depIdx)->fieldIndex(TDEP_NAME));

    QDataWidgetMapper* mapper = new QDataWidgetMapper(this);
    mapper->setModel(model);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->addMapping(ui->comboBoxDepartment, depIdx);
    mapper->addMapping(ui->spinBoxAdjuster, model->fieldIndex(TADJ_ID_KEY));

    connect(ui->tableView->selectionModel(), &QItemSelectionModel::currentRowChanged, mapper, &QDataWidgetMapper::setCurrentModelIndex);
}

AddAdjuster::~AddAdjuster()
{
    delete ui;
}

void AddAdjuster::on_pushButtonAdd_clicked()
{
    QSqlQuery q;

    if (!q.prepare("INSERT INTO " + TABLE_ADJ + "(" + TADJ_ID_KEY + ", " + TADJ_DEP + ", " + TADJ_NAME + ") VALUES(?, ?, ?)")) {
        showError(q.lastError());
        return;
    }

    q.addBindValue(ui->spinBoxAdjuster->value());
    q.addBindValue(getDepIdx());
    q.addBindValue("Введите Ф.И.О.");

    if (!q.exec()) {
        qDebug() << q.lastError().text();
        QMessageBox::warning(this, "", "Не возможно добавить порразделение с данными параметрами.\n"
                                       "Возможно введённые параметры дублируют записи в базе.");
        return;
    }
    if (!model->select()) {
        showError(model->lastError());
        return;
    }
}

void AddAdjuster::on_pushButtonDel_clicked()
{
    auto selectedRows = ui->tableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty() || QMessageBox::question(this, "", "Вы уверены?", QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
        return;

    // Удаление строки из базы данных будет производитсья с помощью SQL-запроса
    QSqlQuery q;

    // Удаление производим по id записи, который передается в качестве аргумента функции
    q.prepare("DELETE FROM " + TABLE_ADJ + " WHERE " + TADJ_ID_KEY + "= :ID ;");
    q.bindValue(":ID", ui->tableView->selectionModel()->selectedRows().first().data());

    // Выполняем удаление
    if (!q.exec()) {
        showError(q.lastError());
        return;
    }
    if (!model->select()) {
        showError(model->lastError());
        return;
    }
}

void AddAdjuster::on_comboBoxDepartment_currentIndexChanged(int /*index*/)
{
    auto v = getDepIdx();
    ui->spinBoxAdjuster->setMinimum(v * 10);
    ui->spinBoxAdjuster->setMaximum(v * 10 + 9);
}

int AddAdjuster::getDepIdx()
{
    auto m = model->relationModel(depIdx);
    return m->data(m->index(ui->comboBoxDepartment->currentIndex(), m->fieldIndex(TDEP_NUMBER))).toInt();
}
