QT += core gui widgets xml sql concurrent printsupport

CONFIG += c++17

TARGET = RoboCell
TEMPLATE = app

INCLUDEPATH += src

HEADERS += \
    src/model/Entities.h \
    src/core/LanguageManager.h \
    src/core/AuthManager.h \
    src/core/LruCache.h \
    src/io/DataRepository.h \
    src/io/XmlProtokollReader.h \
    src/logic/OeeCalculator.h \
    src/ui/LoginDialog.h \
    src/ui/AnlagenTableModel.h \
    src/ui/CustomCharts.h \
    src/ui/LineGraphicsView.h \
    src/ui/AnlageDetailDialog.h \
    src/ui/EditDialogs.h \
    src/ui/MainWindow.h

SOURCES += \
    src/main.cpp \
    src/core/LanguageManager.cpp \
    src/core/AuthManager.cpp \
    src/io/DataRepository.cpp \
    src/io/XmlProtokollReader.cpp \
    src/logic/OeeCalculator.cpp \
    src/ui/LoginDialog.cpp \
    src/ui/AnlagenTableModel.cpp \
    src/ui/CustomCharts.cpp \
    src/ui/LineGraphicsView.cpp \
    src/ui/AnlageDetailDialog.cpp \
    src/ui/EditDialogs.cpp \
    src/ui/MainWindow.cpp
