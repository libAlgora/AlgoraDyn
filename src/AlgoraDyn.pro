########################################################################
# Copyright (C) 2013 - 2019 : Kathrin Hanauer                          #
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
CONFIG += staticlib c++17

ADINFOHDR = $$PWD/algoradyn_info.h
adinfotarget.target =  $$ADINFOHDR
adinfotarget.commands = '$$PWD/../updateInfoHeader $$ADINFOHDR'
adinfotarget.depends = FORCE
PRE_TARGETDEPS += $$ADINFOHDR
QMAKE_EXTRA_TARGETS += adinfotarget

QMAKE_CXXFLAGS_DEBUG += -std=c++17 -O0

QMAKE_CXXFLAGS_STATIC_LIB = # remove -fPIC
QMAKE_CXXFLAGS_RELEASE -= -O1 -O2 -O3
QMAKE_CXXFLAGS_RELEASE += -std=c++17 -DNDEBUG -flto

custom-ar {
  QMAKE_AR += rcs
} else {
  QMAKE_AR = gcc-ar rcs
}

general {
  QMAKE_CXXFLAGS_RELEASE += -O2 -march=x86-64
} else {
  QMAKE_CXXFLAGS_RELEASE += -O3 -march=native -mtune=native
}

debugsymbols {
	QMAKE_CXXFLAGS_RELEASE += -fno-omit-frame-pointer -g
}

profiling {
	QMAKE_CXXFLAGS_DEBUG   += -DCOLLECT_PR_DATA
	QMAKE_CXXFLAGS_RELEASE += -DCOLLECT_PR_DATA
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
include(algorithm.reachability.ss/algorithm.reachability.ss.pri)
include(algorithm.reachability.ss.es/algorithm.reachability.ss.es.pri)
include(algorithm.reachability.ap/algorithm.reachability.ap.pri)
include(pipe/pipe.pri)
include(io/io.pri)
include(graph.dyn.generator/graph.dyn.generator.pri)
