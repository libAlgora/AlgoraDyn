########################################################################
# Copyright (C) 2013 - 2018 : Kathrin Hanauer                          #
#                                                                      #
# This file is part of Algora.                                         #
#                                                                      #
# Algora is free software: you can redistribute it and/or modify       #
# it under the terms of the GNU General Public License as published by #
# the Free Software Foundation, either version 3 of the License, or    #
# (at your option) any later version.                                  #
#                                                                      #
# Algora is distributed in the hope that it will be useful,            #
# but WITHOUT ANY WARRANTY; without even the implied warranty of       #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        #
# GNU General Public License for more details.                         #
#                                                                      #
# You should have received a copy of the GNU General Public License    #
# along with Algora.  If not, see <http://www.gnu.org/licenses/>.      #
#                                                                      #
# Contact information:                                                 #
#   http://algora.xaikal.org                                           #
########################################################################

QT       -= core gui

TARGET = AlgoraDyn
TEMPLATE = lib
CONFIG += staticlib

ADINFOHDR = $$PWD/algoradyn_info.h
adinfotarget.target =  $$ADINFOHDR
adinfotarget.commands = '$$PWD/../updateInfoHeader $$ADINFOHDR'
adinfotarget.depends = FORCE
PRE_TARGETDEPS += $$ADINFOHDR
QMAKE_EXTRA_TARGETS += adinfotarget

QMAKE_CXXFLAGS_DEBUG += -std=c++17 -O0 -DCOLLECT_PR_DATA

QMAKE_CXXFLAGS_STATIC_LIB = # remove -fPIC
QMAKE_CXXFLAGS_RELEASE -= -O3 -O2 -O1
QMAKE_CXXFLAGS_RELEASE += -std=c++17 -O3 -DNDEBUG #-fno-omit-frame-pointer -g -DCOLLECT_PR_DATA

zmiy {
  QMAKE_CXXFLAGS_RELEASE += -march=haswell -mmmx -msse -msse2 -msse3 -mssse3 \
                            -mcx16 -msahf -mmovbe -maes -mpclmul -mpopcnt -mabm \
                            -mfma -mbmi -mbmi2 -mavx -mavx2 -msse4.2 -msse4.1 \
                            -mlzcnt -mrdrnd -mf16c -mfsgsbase -mfxsr -mxsave \
                            -mxsaveopt \
                            --param l1-cache-size=32 --param l1-cache-line-size=64 \
                            --param l2-cache-size=6144 -mtune=haswell \
                            -fstack-protector-strong -Wformat -Wformat-security
} else:zeus {
  QMAKE_CXXFLAGS_RELEASE += -march=ivybridge -mmmx -msse -msse2 -msse3 -mssse3 \
                            -mcx16 -msahf -maes -mpclmul -mpopcnt -mavx -msse4.2 \
                            -msse4.1 -mrdrnd -mf16c -mfsgsbase -mfxsr -mxsave \
                            -mxsaveopt \
                            --param l1-cache-size=32 --param l1-cache-line-size=64 \
                            --param l2-cache-size=20480 -mtune=ivybridge \
                            -fstack-protector-strong -Wformat -Wformat-security
} else {
  QMAKE_CXXFLAGS_RELEASE += -march=native -mtune=native
}

unix {
    target.path = /usr/lib
    INSTALLS += target
}

INCLUDEPATH += $$PWD/../../AlgoraCore/src

DEPENDPATH += $$PWD/../../AlgoraCore/src

HEADERS += \
    $$PWD/algoradyn_info.h

SOURCES += \
    $$PWD/algoradyn_info.cpp

include(graph.dyn/graph.dyn.pri)
include(algorithm/algorithm.pri)
include(algorithm.reach/algorithm.reach.pri)
include(algorithm.reach.es/algorithm.reach.es.pri)
include(pipe/pipe.pri)
include(io/io.pri)
include(graph.dyn.generator/graph.dyn.generator.pri)
