#!/usr/bin/env bash
# Builds scgl_dl.so (OpenGL + SDL2 window wrapper used by scgl.h).
# Needs the SDL2 and OpenGL development packages
# (Debian/Ubuntu: libsdl2-dev libgl-dev).
set -euo pipefail
cd "$(dirname "$0")"

g++ -O2 -std=c++17 -shared -fPIC -fvisibility=hidden -o scgl_dl.so scgl_dl.cpp \
	$(pkg-config --cflags --libs sdl2 gl)
