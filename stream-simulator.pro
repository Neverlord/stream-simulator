QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = stream-simulator
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++14

LIBS += -L/Users/neverlord/caf/build/lib/ -lcaf_core

INCLUDEPATH += /Users/neverlord/caf/libcaf_core/ include/

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/source.cpp \
    src/sink.cpp \
    src/stage.cpp \
    src/entity.cpp \
    src/scatterer.cpp \
    src/gatherer.cpp \
    src/simulant.cpp \
    src/environment.cpp \
    src/simulant_tree_model.cpp \
    src/simulant_tree_item.cpp \
    src/dag_widget.cpp \
    src/edge.cpp \
    src/node.cpp \
    src/entity_details.cpp

HEADERS += \
    include/mainwindow.hpp \
    include/source.hpp \
    include/sink.hpp \
    include/stage.hpp \
    include/entity.hpp \
    include/scatterer.hpp \
    include/gatherer.hpp \
    include/fwd.hpp \
    include/simulant.hpp \
    include/environment.hpp \
    include/simulant_tree_model.hpp \
    include/simulant_tree_item.hpp \
    include/qstr.hpp \
    include/dag_widget.hpp \
    include/edge.hpp \
    include/node.hpp \
    include/entity_details.hpp \
    include/tick_time.hpp

FORMS += \
    ui/mainwindow.ui \
    ui/entity_details.ui
