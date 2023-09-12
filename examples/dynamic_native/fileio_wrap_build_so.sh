#g++ -O3 -s -std=c++17 -shared -fPIC -o fileio_dl.so fileio_dl.cpp
g++ -O0 -std=c++17 -shared -fPIC -o fileio_wrap.so fileio_wrap.cpp
#-rdynamic