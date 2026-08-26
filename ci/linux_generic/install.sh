#!/bin/bash

set -e

# -fno-use-cxa-atexit is needed to build with -coverage
CFLAGS="-Werror" CXXFLAGS="-Werror" ./ci/install.sh
