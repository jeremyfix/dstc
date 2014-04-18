#-------------------------------------------------
#
# Project created by QtCreator 2014-04-12T16:54:54
#
#-------------------------------------------------

QT       += core gui

TARGET = DSTC_Viewer
TEMPLATE = app

CONFIG += link_pkgconfig uitools

QMAKE_CXXFLAGS += -std=c++0x

PKGCONFIG += jsoncpp

SOURCES += main.cpp\
        mainwindow.cpp \
    ontologyviewer.cpp \
    JSONObjects.cpp \
    labelviewer.cpp \
    trackeroutputviewer.cpp

HEADERS  += mainwindow.h \
    JSONObjects.h \
    ontologyviewer.h \
    labelviewer.h \
    trackeroutputviewer.h

FORMS    += mainwindow.ui \
    ontology_viewer.ui \
    labelviewer.ui \
    trackeroutputviewer.ui

RESOURCES += \
    ressource.qrc
