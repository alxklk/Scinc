#!/usr/bin/env bash
set -euo pipefail

g++ -O3 -std=c++17 -shared -fPIC -o system_dl.so system_dl.cpp
