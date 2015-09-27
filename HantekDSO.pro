#-------------------------------------------------
#
# Project created by QtCreator 2015-09-27T01:46:07
#
#-------------------------------------------------

QT       += core gui opengl widgets

TARGET = HantekDSO
TEMPLATE = app
CONFIG += c++-11

SOURCES += main.cpp\
        hantekdso.cpp \
    cursor.cpp \
    fht.cpp \
    glbox.cpp \
    hantekdsoathread.cpp \
    hantekdsoio.cpp \
    hantekdsowidget.cpp \
    hantekdsowidgetbase.cpp \
    sansfont.cpp \
    timecursor.cpp \
    voltagecursor.cpp

HEADERS  += hantekdso.h \
    cursor.h \
    fht.h \
    glbox.h \
    hantekdsoathread.h \
    hantekdsoio.h \
    hantekdsowidget.h \
    hantekdsowidgetbase.h \
    sansfont.h \
    timecursor.h \
    voltagecursor.h

FORMS    +=

DISTFILES += \
    hantekdsoui.rc

LIBS += -lusb-0.1.4

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib
}


QMAKE_CXXFLAGS += -fno-omit-frame-pointer
QMAKE_LFLAGS +=
