# TransitLog — qmake 构建配置（与 MemoRise 的 .pro 工作流一致）
# 也可用 CMakeLists.txt（CI 用）。二选一，均可直接构建。

QT += core gui widgets sql charts

CONFIG += c++17

# 源码按 src/ 相对路径 include（如 #include "app/x.h"）
INCLUDEPATH += src

# MSVC 下强制按 UTF-8 读取源码，避免中文字符串乱码
win32:msvc: CONFIG += utf8_source

TARGET = TransitLog
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/app/authcontroller.cpp \
    src/app/databasemanager.cpp \
    src/app/pbkdf2.cpp \
    src/app/session.cpp \
    src/app/thememanager.cpp \
    src/model/statsservice.cpp \
    src/model/triplistmodel.cpp \
    src/repo/triprepository.cpp \
    src/repo/userrepository.cpp \
    src/ui/appdatetimeedit.cpp \
    src/ui/logindialog.cpp \
    src/ui/mainwindow.cpp \
    src/ui/qflowlayout.cpp \
    src/ui/settingsdialog.cpp \
    src/ui/statswidget.cpp \
    src/ui/trendchartwidget.cpp \
    src/ui/tripcarddelegate.cpp \
    src/ui/tripeditdialog.cpp \
    src/ui/vehiclecatalogwidget.cpp

HEADERS += \
    src/app/authcontroller.h \
    src/app/databasemanager.h \
    src/app/pbkdf2.h \
    src/app/session.h \
    src/app/thememanager.h \
    src/model/entities.h \
    src/model/format.h \
    src/model/statsservice.h \
    src/model/triplistmodel.h \
    src/repo/triprepository.h \
    src/repo/userrepository.h \
    src/ui/appdatetimeedit.h \
    src/ui/logindialog.h \
    src/ui/mainwindow.h \
    src/ui/qflowlayout.h \
    src/ui/settingsdialog.h \
    src/ui/statswidget.h \
    src/ui/trendchartwidget.h \
    src/ui/tripcarddelegate.h \
    src/ui/tripeditdialog.h \
    src/ui/vehiclecatalogwidget.h

# 资源：Tabler 图标字体 + 下拉箭头 PNG + 应用图标
RESOURCES += resources.qrc

# Windows 可执行文件图标（后续加入 app.rc 后启用）
#win32: RC_FILE = app.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
