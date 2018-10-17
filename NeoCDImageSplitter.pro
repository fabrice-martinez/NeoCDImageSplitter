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
    logger.cpp \
    loggerlistwidget.cpp \
    cdromtoc.cpp \
    wavfile.cpp \
    imagewriterworker.cpp

HEADERS  += dialog.h \
    wavfile.h \
    logger.h \
    loggerlistwidget.h \
    cdromtoc.h \
    endian.h \
    packedstruct.h \
    trackindex.h \
    imagewriterworker.h \
    wavstruct.h

FORMS    += dialog.ui

RESOURCES += \
    resources.qrc
