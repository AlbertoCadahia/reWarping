#-------------------------------------------------
#
# Project created by QtCreator 2017-10-23T16:00:20
#
#-------------------------------------------------

QT       += core #gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rewarping
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
            vertex.h

LIBS += -ltiff
        #-lMagick++

#FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

