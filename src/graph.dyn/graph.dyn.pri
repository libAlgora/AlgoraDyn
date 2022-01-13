message("pri file being processed: $$PWD")

HEADERS += \
    $$PWD/dynamicdigraph.h \
    $$PWD/dynamicdigraphoperations.h \
    $$PWD/dynamicdigraphstatistics.h \
    $$PWD/dynamicweighteddigraph.h

SOURCES += \
    $$PWD/dynamicdigraph.cpp \
    $$PWD/dynamicdigraphstatistics.cpp
