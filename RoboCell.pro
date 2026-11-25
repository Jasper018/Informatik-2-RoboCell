QT += core gui widgets xml sql printsupport

CONFIG += c++17

TARGET = RoboCell
TEMPLATE = app

INCLUDEPATH += src

RESOURCES += resources.qrc

HEADERS += \
    src/model/problem.h \
    src/model/datentypen.h \
    src/core/farben.h \
    src/core/einstellungen.h \
    src/core/sprache.h \
    src/core/protokollcache.h \
    src/core/benutzerverwaltung.h \
    src/io/protokollleser.h \
    src/io/datenverwaltung.h \
    src/logic/oeeberechnung.h \
    src/ui/fensterhilfe.h \
    src/ui/anmeldedialog.h \
    src/ui/anlagenmodell.h \
    src/ui/diagramme.h \
    src/ui/linienansicht.h \
    src/ui/bearbeitendialoge.h \
    src/ui/einstellungendialog.h \
    src/ui/anlagedetaildialog.h \
    src/ui/mainwindow.h

SOURCES += \
    src/main.cpp \
    src/model/problem.cpp \
    src/model/datentypen.cpp \
    src/core/farben.cpp \
    src/core/einstellungen.cpp \
    src/core/sprache.cpp \
    src/core/protokollcache.cpp \
    src/core/benutzerverwaltung.cpp \
    src/io/protokollleser.cpp \
    src/io/datenverwaltung.cpp \
    src/logic/oeeberechnung.cpp \
    src/ui/fensterhilfe.cpp \
    src/ui/anmeldedialog.cpp \
    src/ui/anlagenmodell.cpp \
    src/ui/diagramme.cpp \
    src/ui/linienansicht.cpp \
    src/ui/bearbeitendialoge.cpp \
    src/ui/einstellungendialog.cpp \
    src/ui/anlagedetaildialog.cpp \
    src/ui/mainwindow.cpp
