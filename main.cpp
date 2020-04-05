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

    if (/* DISABLES CODE */ (0)) {
        QFont f;
        f.setPointSizeF(10);
        a.setFont(f);
    }

    QElapsedTimer t;
    t.start();

    MainWindow w;
    w.show();

    qDebug() << "elapsed" << t.elapsed() / 1000.0 << "sec.";

    return a.exec();
}
