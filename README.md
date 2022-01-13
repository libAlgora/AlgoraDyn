[![pipeline status](https://gitlab.com/libalgora/AlgoraDyn/badges/master/pipeline.svg)](https://gitlab.com/libalgora/AlgoraDyn/commits/master)

# Algora|Dyn

**Algora|Dyn** is the dynamic graph and algorithms library of the [**Algora**
project](https://libalgora.gitlab.io).

It currently provides a dynamic directed graph data structure, i.e., a directed
graph that undergoes a series of updates, as well as
a set of dynamic single-source reachability algorithms.

## Building

**Algora|Dyn** is written in C++17.
Implementations are based on the STL and additionally use selected boost
libraries.
The building process employs `qmake` version 5.

On Debian/Ubuntu, all dependencies can be installed by running: `# apt install
qt5-qmake libboost-dev`.
On Fedora, run `# dnf install qt5-qtbase-devel boost-devel`.
On FreeBSD, run `# pkg install qt5-qmake boost-libs`.

**Algora|Dyn** is built on top of the Algora core library
[**Algora|Core**](https://gitlab.com/libalgora/AlgoraCore).
Before you can compile **Algora|Dyn**, you therefore first need to
clone the core library.
To allow the build process to run smoothly, create
something like the following directory structure:
```
Algora
|- AlgoraCore
|- AlgoraDyn
```
This can be achieved, e.g., by running these commands:
```
$ mkdir Algora && cd Algora
$ git clone https://gitlab.com/libalgora/AlgoraCore
$ git clone https://gitlab.com/libalgora/AlgoraDyn
```

**Algora|Dyn** comes with an
`easyCompile` script that creates the necessary build directories and
compiles the library.
All you need to do now is:
```
$ cd AlgoraDyn
$ ./easyCompile
```
The compiled library can then be found in the `build/Debug` and `build/Release`
subdirectories.


## License

**Algora** and **Algora|Dyn** is free software and licensed under the
GNU General Public License version 3.
See also the file `COPYING`.

## Contributors

- Kathrin Hanauer (project initiator & maintainer)
