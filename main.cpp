#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTranslator>

void translation(QApplication* app)
{
    const QString loc(QLocale().name().left(2));
    qDebug() << "locale:" << loc;
    QString trFolder;
    trFolder = (qApp->applicationDirPath() + "/translations/");
    auto translator = [app](const QString& path) {
        if (QFile::exists(path)) {
            QTranslator* pTranslator = new QTranslator();
            if (pTranslator->load(path))
                app->installTranslator(pTranslator);
            else
                delete pTranslator;
        }
    };
    translator(trFolder + "qtbase_" + loc + ".qm");
    translator(trFolder + "qt_" + loc + ".qm");
}

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    translation(&a);

    QFont f;
    f.setPointSizeF(10);
    a.setFont(f);

    MainWindow w;
    w.show();
    return a.exec();
}
