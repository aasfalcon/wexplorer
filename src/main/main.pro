QT += core gui widgets

TARGET = ../samplebrowser
TEMPLATE = app
CONFIG += c++11 precompile_header
PRECOMPILED_HEADER = stable.h

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    driverdialog.cpp \
    browser.cpp \
    application.cpp

HEADERS += \
    mainwindow.h \
    driverdialog.h \
    browser.h \
    application.h \
    config.h

FORMS += \
    mainwindow.ui \
    driverdialog.ui \
    browser.ui

LIBS += \
    -L../engine -lengine \
    -L../widgets -lwidgets \
    -L../shared -lweplsh \
    -L../sound -lsound \
    \
    -L../../bundle/qvumeter -lqvumeter \
    -L../../bundle/WaveformWidget -lWaveformWidget \
    -L../../bundle/audiowaveform -laudiowaveform \
    -L../../bundle/vpiano -lvpiano \
    \
    -lsndfile \
    -llog4cplus \
    -lpng12 \
    -lltdl

RESOURCES += \
    assets/config.qrc \
    assets/icons.qrc \
    assets/themes.qrc

INCLUDEPATH += ../ ../../bundle ../../vendor/includes ../plugins ../sound

PRE_TARGETDEPS += ../../bundle ../engine ../widgets ../shared ../plugins ../sound
