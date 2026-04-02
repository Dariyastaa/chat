QT += widgets network
CONFIG += c++11

QMAKE_POST_LINK += $$quote($$[QT_INSTALL_BINS]/windeployqt.exe $$OUT_PWD/release/client.exe)

TARGET = client

SOURCES += \
    main.cpp \
    clientwindow.cpp

HEADERS += \
    clientwindow.h

FORMS += \
    clientwindow.ui