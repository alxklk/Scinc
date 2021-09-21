#!/usr/bin/env bash

g++ -static-libgcc -static-libstdc++ -O3 -std=c++17 $1 -o $1.out /home/klk/wrk/Cumulus/src/ws_x11.cpp -I/home/klk/wrk/Cumulus/src -I/home/klk/wrk/Cumulus/Scrip -ldl -lGL -lX11 -lXext -lasound
