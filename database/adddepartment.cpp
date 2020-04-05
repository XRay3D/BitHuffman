#include "adddepartment.h"
#include "ui_adddepartment.h"

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
        case 0: {
            QSpinBox* sb = new QSpinBox(parent);
            sb->setFrame(false);
            sb->setButtonSymbols(QSpinBox::NoButtons);
            sb->setReadOnly(true);
            return sb;
        }
        case 1: {
            QLineEdit* le = new QLineEdit(parent);
            le->setFrame(false);
            le->setText(index.data().toString());
            return le;
        }
        }
        return QSqlRelationalDelegate::createEditor(parent, option, index);
    }
};

AddDepartment::AddDepartment(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AddDepartment)
{
    ui->setupUi(this);

    // Create the data model:
    model = new QSqlTableModel(ui->tableView);

    model->setEditStrategy(QSqlTableModel::OnFieldChange);
    model->setTable(TABLE_DEPARTMENT);

    // Set the header captions:
    model->setHeaderData(model->fieldIndex(TDEP_NUMBER), Qt::Horizontal, "№ подразделения");
    model->setHeaderData(model->fieldIndex(TDEP_NAME), Qt::Horizontal, "Название");

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
    ui->tableView->horizontalHeader()->setSectionResizeMode(model->fieldIndex(TDEP_NUMBER), QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(model->fieldIndex(TDEP_NAME), QHeaderView::Stretch);
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableView->verticalHeader()->setDefaultSectionSize(QFontMetrics(QFont()).height());
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->setEditTriggers(QAbstractItemView::DoubleClicked);

    QDataWidgetMapper* mapper = new QDataWidgetMapper(this);
    mapper->setModel(model);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    //    mapper->setItemDelegate(new BookDelegate(this));
    mapper->addMapping(ui->spinBoxNumber, model->fieldIndex(TDEP_NUMBER));

    connect(ui->tableView->selectionModel(), &QItemSelectionModel::currentRowChanged, mapper, &QDataWidgetMapper::setCurrentModelIndex);

    //ui->tableView->setCurrentIndex(model->index(0, 0));
}

AddDepartment::~AddDepartment()
{
    delete ui;
}

void AddDepartment::on_pushButtonAdd_clicked()
{
    QSqlQuery q;

    if (!q.prepare("INSERT INTO " + TABLE_DEPARTMENT + "(" + TDEP_NUMBER + ", " + TDEP_NAME + ") VALUES(?, ?)")) {
        showError(q.lastError());
        return;
    }

    q.addBindValue(ui->spinBoxNumber->value());
    q.addBindValue("Введите название");
    if (!q.exec()) {
        QMessageBox::warning(this, "", "Не возможно добавить порразделение с данными параметрами.\n"
                                       "Возможно введённые параметры дублируют записи в базе.");
        return;
    }

    if (!model->select()) {
        showError(model->lastError());
        return;
    }
}

void AddDepartment::on_pushButtonDel_clicked()
{
    auto selectedRows = ui->tableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty() || QMessageBox::question(this, "", "Вы уверены?\nДанное действо удалит также регулировщиков.", QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
        return;

    const int id { selectedRows.first().data().toInt() };
    // Удаление строки из базы данных будет производитсья с помощью SQL-запроса
    QSqlQuery q;

    // Удаление производим по id записи, который передается в качестве аргумента функции
    q.prepare("DELETE FROM " + TABLE_DEPARTMENT + " WHERE " + TDEP_NUMBER + "= :ID ;");
    q.bindValue(":ID", id);

    // Выполняем удаление
    if (!q.exec()) {
        showError(q.lastError());
        return;
    }
    if (!model->select()) {
        showError(model->lastError());
        return;
    }
    { // Удаление строки из базы данных будет производитсья с помощью SQL-запроса
        QSqlQuery q;

        // Удаление производим по id записи, который передается в качестве аргумента функции
        q.prepare("DELETE FROM " + TABLE_ADJ + " WHERE " + TADJ_DEP + "= :ID ;");
        q.bindValue(":ID", id);

        // Выполняем удаление
        if (!q.exec()) {
            showError(q.lastError());
            return;
        }
    }
}
