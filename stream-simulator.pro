QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = stream-simulator
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++14

LIBS += -L/Users/neverlord/caf/build/lib/ -lcaf_core

INCLUDEPATH += /Users/neverlord/caf/libcaf_core/ include/

SOURCES += \
    src/dag_widget.cpp \
    src/edge.cpp \
    src/entity.cpp \
    src/entity_details.cpp \
    src/environment.cpp \
    src/gatherer.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/node.cpp \
    src/rate_controlled_sink.cpp \
    src/rate_controlled_source.cpp \
    src/simulant.cpp \
    src/simulant_tree_item.cpp \
    src/simulant_tree_model.cpp \
    src/sink.cpp \
    src/source.cpp \
    src/stage.cpp \
    src/term_gatherer.cpp \
    src/term_scatterer.cpp

HEADERS += \
    include/critical_section.hpp \
    include/dag_widget.hpp \
    include/edge.hpp \
    include/entity.hpp \
    include/entity_details.hpp \
    include/environment.hpp \
    include/fwd.hpp \
    include/gatherer.hpp \
    include/mainwindow.hpp \
    include/node.hpp \
    include/qstr.hpp \
    include/rate_controlled_sink.hpp \
    include/rate_controlled_source.hpp \
    include/scatterer.hpp \
    include/simulant.hpp \
    include/simulant_tree_item.hpp \
    include/simulant_tree_model.hpp \
    include/sink.hpp \
    include/source.hpp \
    include/stage.hpp \
    include/term_gatherer.hpp \
    include/tick_time.hpp

FORMS += \
    ui/mainwindow.ui \
    ui/entity_details.ui
