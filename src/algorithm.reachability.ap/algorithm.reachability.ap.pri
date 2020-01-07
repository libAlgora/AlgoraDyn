message("pri file being processed: $$PWD")

HEADERS += \
    $$PWD/dynamicallpairsreachabilityalgorithm.h \
    $$PWD/staticbfsapreachabilityalgorithm.h \
    $$PWD/staticdfsapreachabilityalgorithm.h \
    $$PWD/supportiveverticesdynamicallpairsreachabilityalgorithm.h

SOURCES += \
    $$PWD/dynamicallpairsreachabilityalgorithm.cpp \
    $$PWD/staticbfsapreachabilityalgorithm.cpp \
    $$PWD/staticdfsapreachabilityalgorithm.cpp \
    $$PWD/supportiveverticesdynamicallpairsreachabilityalgorithm.cpp
