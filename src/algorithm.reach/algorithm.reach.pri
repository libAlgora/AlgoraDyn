message("pri file being processed: $$PWD")

HEADERS += \
    $$PWD/dynamicssreachalgorithm.h \
    $$PWD/simpleincssreachalgorithm.h \
    $$PWD/estree.h \
    $$PWD/cachingbfsssreachalgorithm.h \
    $$PWD/lazybfsssreachalgorithm.h \
    $$PWD/staticbfsssreachalgorithm.h

SOURCES += \
    $$PWD/simpleincssreachalgorithm.cpp \
    $$PWD/estree.cpp \
    $$PWD/cachingbfsssreachalgorithm.cpp \
    $$PWD/lazybfsssreachalgorithm.cpp \
    $$PWD/staticbfsssreachalgorithm.cpp
