#!/usr/bin/env bash

g++ -O0 -g -std=c++17 $1 -o $1.out /home/klk/wrk/Cumulus/src/ws_x11.cpp -I/home/klk/wrk/Cumulus/src -I/home/klk/wrk/Cumulus/Scrip -ldl -lGL -lX11 -lXext -lasound