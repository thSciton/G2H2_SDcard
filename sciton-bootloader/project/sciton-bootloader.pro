QT += core
QT -= gui

TARGET = sciton-bootloader
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += ../src/main.cpp \
    ../src/phytecmodule.cpp

HEADERS += ../src/h/phytecmodule.h

INCLUDEPATH += /opt/reach/1.6/sysroots/cortexa9hf-vfp-neon-reach-linux-gnueabi/usr/include
INCLUDEPATH += /opt/reach/1.6/sysroots/cortexa9hf-vfp-neon-reach-linux-gnueabi/usr/include/c++/4.9.1
INCLUDEPATH += ../src/h
INCLUDEPATH += ../src

linux-* {
target.path = /home/root
INSTALLS += target
}
