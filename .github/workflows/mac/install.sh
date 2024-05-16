#!/bin/bash

set -e

ccache -M 200M

CC="clang" CXX="clang++" CFLAGS="" CXXFLAGS="" CMAKE_BUILD_TYPE=RelWithDebInfo ./travis/install.sh
