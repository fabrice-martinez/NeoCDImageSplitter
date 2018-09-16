#-------------------------------------------------
#
# Project created by QtCreator 2015-09-03T20:20:31
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NeoCDImageSplitter
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
        dialog.cpp \
    edc.cpp \
    qpropertymemory.cpp

HEADERS  += dialog.h \
    edc.h \
    qpropertymemory.h \
    wavfile.h

FORMS    += dialog.ui

RESOURCES += \
    resources.qrc
