#-------------------------------------------------
#
# Project created by QtCreator 2016-07-02T12:15:47
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = netTestTray
TEMPLATE = app

CONFIG += c++14


SOURCES += main.cpp\
        widget.cpp \
    Pinger/Pinger.cpp

HEADERS  += widget.h \
    Pinger/icmp_header.hpp \
    Pinger/ipv4_header.hpp \
    Pinger/Pinger.h

INCLUDEPATH += $$PWD/Pinger
INCLUDEPATH += "C:/Users/nikbol/Documents/visual studio 2015/Projects/crcRepairing/packages/boost.1.61.0.0/lib/native/include"

win32-g++ {
LIBS += "C:/Boost/lib/libboost_system-mgw49-mt-d-1_61.a" -lWs2_32
}

win32-msvc* {
CONFIG += embed_manifest_exe
QMAKE_LFLAGS_WINDOWS += $$quote( /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\" )

LIBS += "-LC:/Users/nikbol/Documents/visual studio 2015/Projects/crcRepairing/packages/boost_system-vc140.1.61.0.0/lib/native/address-model-32/lib"
}

RESOURCES += \
    icons.qrc
