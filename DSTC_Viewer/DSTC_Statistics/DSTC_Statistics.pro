
TARGET = ../db_statistics
TEMPLATE = app


CONFIG += link_pkgconfig

PKGCONFIG += jsoncpp

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_LFLAGS_RELEASE -= -O1


SOURCES = main.cpp

HEADERS =

INCLUDEPATH += ../JSONParser/
LIBS += -L../JSONParser/ -lJSONParser
