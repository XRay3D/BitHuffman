QT       += core gui sql printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GenSerNum

CONFIG += c++17

DESTDIR = $$_PRO_FILE_PWD_/bin


# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    database/adddepartment.cpp \
    database/addadjuster.cpp \
    database/addsernums.cpp \
    database/database.cpp \
    main.cpp \
    mainwindow.cpp \
    model.cpp \
    types.cpp

HEADERS += \
    database/adddepartment.h \
    database/addadjuster.h \
    database/addsernums.h \
    database/database.h \
    mainwindow.h \
    model.h \
    types.h

FORMS += \
    database/adddepartment.ui \
    database/addadjuster.ui \
    database/addsernums.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Hff.java \
    bin/database.bin \
    bin/database.db \
    huffman.cpp \
    pQueue.cpp \
    pQueue.h \
    huffman.h \
