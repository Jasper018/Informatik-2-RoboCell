cmake_minimum_required(VERSION 3.16)

project(RoboCell VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Xml Sql Concurrent PrintSupport)

set(PROJECT_SOURCES
    src/main.cpp
    src/model/Entities.h
    src/core/LanguageManager.h
    src/core/LanguageManager.cpp
    src/core/AuthManager.h
    src/core/AuthManager.cpp
    src/core/LruCache.h
    src/io/DataRepository.h
    src/io/DataRepository.cpp
    src/io/XmlProtokollReader.h
    src/io/XmlProtokollReader.cpp
    src/logic/OeeCalculator.h
    src/logic/OeeCalculator.cpp
    src/ui/LoginDialog.h
    src/ui/LoginDialog.cpp
    src/ui/AnlagenTableModel.h
    src/ui/AnlagenTableModel.cpp
    src/ui/CustomCharts.h
    src/ui/CustomCharts.cpp
    src/ui/LineGraphicsView.h
    src/ui/LineGraphicsView.cpp
    src/ui/AnlageDetailDialog.h
    src/ui/AnlageDetailDialog.cpp
    src/ui/EditDialogs.h
    src/ui/EditDialogs.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

add_executable(RoboCell WIN32 ${PROJECT_SOURCES})

target_include_directories(RoboCell PRIVATE src)

target_link_libraries(RoboCell PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Xml
    Qt6::Sql
    Qt6::Concurrent
    Qt6::PrintSupport
)

# Optional automated test executable
add_executable(RoboCellTests
    tests/test_main.cpp
    src/model/Entities.h
    src/core/LanguageManager.h
    src/core/LanguageManager.cpp
    src/core/AuthManager.h
    src/core/AuthManager.cpp
    src/core/LruCache.h
    src/io/DataRepository.h
    src/io/DataRepository.cpp
    src/io/XmlProtokollReader.h
    src/io/XmlProtokollReader.cpp
    src/logic/OeeCalculator.h
    src/logic/OeeCalculator.cpp
)

target_include_directories(RoboCellTests PRIVATE src)
target_link_libraries(RoboCellTests PRIVATE
    Qt6::Core
    Qt6::Xml
    Qt6::Sql
    Qt6::Concurrent
)
