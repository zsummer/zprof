#!/bin/bash
if [ ! -d build_mac ]; then
mkdir build_mac
fi
cd build_mac
cmake -GXcode ../ 
cd ../
