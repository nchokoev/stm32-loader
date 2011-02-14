#-------------------------------------------------
#
# Project created by QtCreator 2009-11-21T17:06:44
#
#-------------------------------------------------

LIBS += -L./ -lftd2xx

TARGET = fttest
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
			ftd2xx.h

FORMS    += mainwindow.ui
