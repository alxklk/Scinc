#!/usr/bin/env bash
# Builds http_dl.so (HTTP GET + JSON helpers used by http.h).
# libcurl is loaded at run time (Debian/Ubuntu package libcurl4), so no curl
# development headers are needed.
set -euo pipefail
cd "$(dirname "$0")"

g++ -O2 -std=c++17 -shared -fPIC -fvisibility=hidden -o http_dl.so http_dl.cpp -ldl -lpthread
