#!/usr/bin/env bash
# Builds img_dl.so, the picture decoder t2fc uses for its picture viewer and quick
# view. WebP needs libwebp (Debian/Ubuntu: libwebp-dev); without it the other
# formats still work. t2fc also needs ../tui/term_dl.so (../tui/build_so.sh).
set -euo pipefail
cd "$(dirname "$0")"

if pkg-config --exists libwebpdemux libwebp 2>/dev/null; then
	g++ -O2 -std=c++17 -shared -fPIC -DHAVE_WEBP -o img_dl.so img_dl.cpp \
		$(pkg-config --cflags --libs libwebpdemux libwebp) -lpthread
	echo "img_dl.so built with WebP"
else
	g++ -O2 -std=c++17 -shared -fPIC -o img_dl.so img_dl.cpp -lpthread
	echo "img_dl.so built without WebP (install libwebp-dev for it)"
fi
