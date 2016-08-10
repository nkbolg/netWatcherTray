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

win32-g++ {
LIBS += "C:/Boost/lib/libboost_system-mgw49-mt-d-1_61.a" -lWs2_32
}

win32-msvc* {
CONFIG += embed_manifest_exe
QMAKE_LFLAGS_WINDOWS += $$quote( /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\" )

    !contains(QMAKE_TARGET.arch, x86_64) {
        message("x86 build")
        LIBS += "-L$$PWD/Pinger/libs/native/address-model-32/lib"

    } else {
        message("x64 build")
        LIBS += "-L$$PWD/Pinger/libs/native/address-model-64/lib"
    }

}

RESOURCES += \
    icons.qrc
