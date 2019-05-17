message("pri file being processed: $$PWD")

HEADERS += \
    $$PWD/konectnetworkreader.h \
    $$PWD/streamdynamicdigraphreader.h \
    $$PWD/dynamicdigraphqueryreader.h

SOURCES += \
    $$PWD/konectnetworkreader.cpp \
    $$PWD/dynamicdigraphqueryreader.cpp
