#-------------------------------------------------
#
# Project created by QtCreator 2018-12-27T11:18:28
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = IDT_Demo
TEMPLATE = app


SOURCES += main.cpp \
        mainwindow.cpp \
    zmod44xx.cpp \
    hicom/hicom.cpp \
    hicom/hicom_i2c.cpp \
    edp/cJSON.cpp \
    edp/EdpKit.cpp

HEADERS  += mainwindow.h \
    libs/eco2.h \
    libs/iaq.h \
    libs/odor.h \
    libs/r_cda.h \
    libs/tvoc.h \
    hicom/FTCI2C.h \
    hicom/hicom.h \
    hicom/hicom_i2c.h \
    zmod44xx.h \
    zmod44xx_config.h \
    zmod44xx_types.h \
    edp/cJSON.h \
    edp/Common.h \
    edp/EdpKit.h

FORMS    += mainwindow.ui

LIBS += \
    "-LC:\Users\zhaojuntao\Desktop\IDT_Demo\libs" -leco2_x64_win \
    "-LC:\Users\zhaojuntao\Desktop\IDT_Demo\libs" -liaq_x64_win \
    "-LC:\Users\zhaojuntao\Desktop\IDT_Demo\libs" -lr_cda_x64_win \
    "-LC:\Users\zhaojuntao\Desktop\IDT_Demo\libs" -ltvoc_x64_win \
    C:\Users\zhaojuntao\Desktop\IDT_Demo\libs\eco2_x86_win.lib \
    C:\Users\zhaojuntao\Desktop\IDT_Demo\libs\iaq_x86_win.lib \
    C:\Users\zhaojuntao\Desktop\IDT_Demo\libs\r_cda_x86_win.lib \
    C:\Users\zhaojuntao\Desktop\IDT_Demo\libs\tvoc_x86_win.lib \
    C:\Users\zhaojuntao\Desktop\build-IDT_Demo-Desktop_Qt_5_7_0_MinGW_32bit-Debug\debug\RSRFTCI2C.dll

RESOURCES += \
    idt_rcs.qrc

RC_FILE = proj.rc
