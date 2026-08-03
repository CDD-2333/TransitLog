cmake_minimum_required(VERSION 3.16)
project(TransitLog VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

option(BUILD_TESTS "构建冒烟测试" OFF)

set(TRANSITLOG_SOURCES
    src/app/authcontroller.cpp
    src/app/authcontroller.h
    src/app/databasemanager.cpp
    src/app/databasemanager.h
    src/app/pbkdf2.cpp
    src/app/pbkdf2.h
    src/app/session.cpp
    src/app/session.h
    src/app/thememanager.cpp
    src/app/thememanager.h
    src/model/entities.h
    src/model/format.h
    src/model/triplistmodel.cpp
    src/model/triplistmodel.h
    src/repo/triprepository.cpp
    src/repo/triprepository.h
    src/repo/userrepository.cpp
    src/repo/userrepository.h
    src/ui/appdatetimeedit.cpp
    src/ui/appdatetimeedit.h
    src/ui/chartwidget.cpp
    src/ui/chartwidget.h
    src/ui/logindialog.cpp
    src/ui/logindialog.h
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/settingsdialog.cpp
    src/ui/settingsdialog.h
    src/ui/statswidget.cpp
    src/ui/statswidget.h
    src/ui/tripcarddelegate.cpp
    src/ui/tripcarddelegate.h
    src/ui/tripeditdialog.cpp
    src/ui/tripeditdialog.h
)

# 核心库（不含 main），供主程序与测试共同链接；Qt 依赖 PUBLIC 传播
qt_add_library(transitlog_core STATIC ${TRANSITLOG_SOURCES})
target_include_directories(transitlog_core PUBLIC src)
target_link_libraries(transitlog_core PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql)

# 资源(.qrc)直接进可执行目标而非静态库：
# 静态库里的 qrc 资源注册对象可能被链接器丢弃，导致资源加载失败。
qt_add_executable(TransitLog src/main.cpp resources.qrc)
target_link_libraries(TransitLog PRIVATE transitlog_core)

if(BUILD_TESTS)
    qt_add_executable(TransitLogDataSmoke tests/data_smoke.cpp)
    target_link_libraries(TransitLogDataSmoke PRIVATE transitlog_core)

    qt_add_executable(TransitLogGuiSmoke tests/gui_smoke.cpp resources.qrc)
    target_link_libraries(TransitLogGuiSmoke PRIVATE transitlog_core)
endif()

# MSVC：源码为 UTF-8，避免中文字符串乱码
if(MSVC)
    target_compile_options(transitlog_core PRIVATE /utf-8)
    target_compile_options(TransitLog PRIVATE /utf-8)
    if(BUILD_TESTS)
        target_compile_options(TransitLogDataSmoke PRIVATE /utf-8)
        target_compile_options(TransitLogGuiSmoke PRIVATE /utf-8)
    endif()
endif()

set_target_properties(TransitLog PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
