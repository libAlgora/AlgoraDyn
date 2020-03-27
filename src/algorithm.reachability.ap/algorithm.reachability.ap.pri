message("pri file being processed: $$PWD")

HEADERS += \
    $$PWD/dynamicallpairsreachabilityalgorithm.h \
    $$PWD/staticbfsapreachabilityalgorithm.h \
    $$PWD/staticdbfsapreachabilityalgorithm.h \
    $$PWD/staticdfsapreachabilityalgorithm.h \
    $$PWD/supportiveverticesdynamicallpairsreachabilityalgorithm.h \
    $$PWD/supportiveverticessloppysccsapralgorithm.h

SOURCES += \
    $$PWD/dynamicallpairsreachabilityalgorithm.cpp \
    $$PWD/staticbfsapreachabilityalgorithm.cpp \
    $$PWD/staticdbfsapreachabilityalgorithm.cpp \
    $$PWD/staticdfsapreachabilityalgorithm.cpp \
    $$PWD/supportiveverticesdynamicallpairsreachabilityalgorithm.cpp \
    $$PWD/supportiveverticessloppysccsapralgorithm.cpp
