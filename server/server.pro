QT += widgets network
CONFIG += c++11

QMAKE_POST_LINK += $$quote($$[QT_INSTALL_BINS]/windeployqt.exe $$OUT_PWD/release/server.exe)

TARGET = server

SOURCES += \
    main.cpp \
    serverwindow.cpp

HEADERS += \
    serverwindow.h

FORMS += \
    serverwindow.ui