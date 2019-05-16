#!/bin/sh
rm -rf ./debug
mkdir ./debug
cd ./debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make