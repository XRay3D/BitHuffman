#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "types.h"
#include <QMainWindow>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QtSql>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class DataBase;
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_pbGen_clicked();
    void on_groupBoxFormat_toggled(bool arg1);
    void on_groupBoxFormatBit_toggled(bool arg1);
    void on_sbxSerNumCoded_valueChanged(int arg1);
    void on_pbReset_clicked();
    void on_pbFind_clicked();
    void on_tvOrders_doubleClicked(const QModelIndex& index);

private:
    Ui::MainWindow* ui;

    DataBase* db;
    QSqlRelationalTableModel* modelOrder;
    QSqlTableModel* modelSerNum;

    void toEncSerNum() {}
    void fromEncSerNum() {}
    void printDialog();
    void tos();
    void showOrders();
    void setOrderFilter(const QString& filter);
};

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

#endif // MAINWINDOW_H
