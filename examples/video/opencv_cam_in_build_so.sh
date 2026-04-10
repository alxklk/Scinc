#!/usr/bin/env bash
set -euo pipefail

opencv_pkg="${OPENCV_PKG:-opencv4}"
if ! pkg-config --exists "${opencv_pkg}"; then
	opencv_pkg="opencv"
fi

g++ -O3 -std=c++17 -shared -fPIC -o opencv_cam_in_dl.so opencv_cam_in_dl.cpp $(pkg-config --cflags --libs "${opencv_pkg}")
