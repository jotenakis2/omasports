QT += core gui qml quick quickcontrols2 testlib network
CONFIG += c++17 testcase
TARGET = eventcard_geometry_tests
TEMPLATE = app

INCLUDEPATH += ../src
DEFINES += SRCDIR=\\\"$$PWD\\\"

SOURCES += eventcard_geometry_tests.cpp
RESOURCES += ../src/resources.qrc test_assets.qrc
