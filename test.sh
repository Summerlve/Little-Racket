#!/bin/sh
rm -rf ./build
mkdir ./build
cd ./build
cmake -DCMAKE_BUILD_TYPE=Test ..
make
ctest --output-on-failure