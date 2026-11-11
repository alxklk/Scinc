#!/usr/bin/env bash
# Builds img_dl.so, the picture decoder t2fc uses for its picture viewer and quick
# view, and arc_dl.so, which shows archives as folders. libwebp is opened at run time, so the library needs nothing beyond the C
# and C++ runtime (linked in statically, for machines with an older one); the
# libwebp headers (Debian/Ubuntu: libwebp-dev) are needed only to build WebP in.
# t2fc also needs ../tui/term_dl.so (../tui/build_so.sh).
set -euo pipefail
cd "$(dirname "$0")"

FLAGS="-O2 -std=c++17 -shared -fPIC -static-libstdc++ -static-libgcc"
if [ -f /usr/include/webp/demux.h ]; then
	g++ $FLAGS -DHAVE_WEBP -o img_dl.so img_dl.cpp -ldl -lpthread
	echo "img_dl.so built with WebP"
else
	g++ $FLAGS -o img_dl.so img_dl.cpp -ldl -lpthread
	echo "img_dl.so built without WebP (install libwebp-dev for it)"
fi

# arc_dl.so: archives as read-only folders; libarchive (libarchive.so.13) is opened
# at run time, so no headers are needed
g++ $FLAGS -o arc_dl.so arc_dl.cpp -ldl
echo "arc_dl.so built"
