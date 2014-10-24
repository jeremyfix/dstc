#-------------------------------------------------
#
# Project created by QtCreator 2014-04-12T16:54:54
#
#-------------------------------------------------

QT       += core gui

TARGET = dstc-viewer
TEMPLATE = app

CONFIG += uitools link_pkgconfig

PKGCONFIG += jsoncpp

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_LFLAGS_RELEASE -= -O1

SOURCES += main.cpp\
        mainwindow.cpp \
    ontologyviewer.cpp \
    labelviewer.cpp \
    trackeroutputviewer.cpp

HEADERS  += mainwindow.h \
    ontologyviewer.h \
    labelviewer.h \
    trackeroutputviewer.h

FORMS    += mainwindow.ui \
    ontology_viewer.ui \
    labelviewer.ui \
    trackeroutputviewer.ui

RESOURCES += \
    ressource.qrc

INCLUDEPATH += ../JSONParser/
LIBS += -L../JSONParser/ -lJSONParser
