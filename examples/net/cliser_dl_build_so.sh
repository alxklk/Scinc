#g++ -O3 -s -std=c++17 -shared -fPIC -o fileio_dl.so fileio_dl.cpp
g++ -O0 -std=c++17 -shared -fPIC -o cliser_dl.so cliser.cpp
#-rdynamic