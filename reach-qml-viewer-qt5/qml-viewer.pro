#-------------------------------------------------
#
# Project created by QtCreator 2012-09-07T15:59:59
#
#-------------------------------------------------

# this needs to stay in sync with the Makefile
VERSION = 1.1.0

QT       += network quick

LIBS += -lasound

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qml-viewer
TEMPLATE = app
target.path = /application/bin
INSTALLS += target

# add #define for the version
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += main.cpp\
    connection.cpp \
    mainview.cpp \
    messagehandler.cpp \
    settings.cpp \
    screen.cpp \
    watchdog.cpp \
    beep.cpp

HEADERS  += \
    connection.h \
    mainview.h \
    messagehandler.h \
    systemdefs.h \
    settings.h \
    screen.h \
    watchdog.h \
    beep.h

