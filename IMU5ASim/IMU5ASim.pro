QT += core gui widgets serialport charts

CONFIG += c++17
TARGET  = IMU5ASim
TEMPLATE = app

# Suppress deprecation warnings from Qt internals
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    packetbuilder.cpp \
    serialtransport.cpp \
    datareader.cpp \
    simulatorengine.cpp \
    guidedialog.cpp

HEADERS += \
    mainwindow.h \
    packetbuilder.h \
    serialtransport.h \
    datareader.h \
    simulatorengine.h \
    guidedialog.h \
    imuframe.h \
    scaleconfig.h

# Windows: link kernel32 for QueryPerformanceCounter
win32: LIBS += -lkernel32

# Output directories
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/build/debug
} else {
    DESTDIR = $$PWD/build/release
}
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
RCC_DIR     = $$PWD/build/rcc
UI_DIR      = $$PWD/build/ui
