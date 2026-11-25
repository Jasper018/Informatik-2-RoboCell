cmake_minimum_required(VERSION 3.16)

project(RoboCell VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Xml Sql PrintSupport)

set(CORE_AND_LOGIC_SOURCES
    src/model/problem.h
    src/model/problem.cpp
    src/model/datentypen.h
    src/model/datentypen.cpp
    src/core/farben.h
    src/core/farben.cpp
    src/core/einstellungen.h
    src/core/einstellungen.cpp
    src/core/sprache.h
    src/core/sprache.cpp
    src/core/protokollcache.h
    src/core/protokollcache.cpp
    src/core/benutzerverwaltung.h
    src/core/benutzerverwaltung.cpp
    src/io/protokollleser.h
    src/io/protokollleser.cpp
    src/io/datenverwaltung.h
    src/io/datenverwaltung.cpp
    src/logic/oeeberechnung.h
    src/logic/oeeberechnung.cpp
    resources.qrc
)

set(UI_SOURCES
    src/ui/fensterhilfe.h
    src/ui/fensterhilfe.cpp
    src/ui/anmeldedialog.h
    src/ui/anmeldedialog.cpp
    src/ui/anlagenmodell.h
    src/ui/anlagenmodell.cpp
    src/ui/diagramme.h
    src/ui/diagramme.cpp
    src/ui/linienansicht.h
    src/ui/linienansicht.cpp
    src/ui/bearbeitendialoge.h
    src/ui/bearbeitendialoge.cpp
    src/ui/einstellungendialog.h
    src/ui/einstellungendialog.cpp
    src/ui/anlagedetaildialog.h
    src/ui/anlagedetaildialog.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.cpp
)

add_executable(RoboCell WIN32
    src/main.cpp
    ${CORE_AND_LOGIC_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(RoboCell PRIVATE src)
target_link_libraries(RoboCell PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Xml
    Qt6::Sql
    Qt6::PrintSupport
)

add_executable(RoboCellTests
    tests/test_main.cpp
    ${CORE_AND_LOGIC_SOURCES}
)

target_include_directories(RoboCellTests PRIVATE src)
target_link_libraries(RoboCellTests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Xml
    Qt6::Sql
)
