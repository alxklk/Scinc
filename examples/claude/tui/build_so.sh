#!/usr/bin/env bash
# Builds term_dl.so, the terminal input library used by term.h.
set -euo pipefail
cd "$(dirname "$0")"

g++ -O2 -std=c++17 -shared -fPIC -pthread -o term_dl.so term_dl.cpp
