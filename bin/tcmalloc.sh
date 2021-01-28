#!/bin/bash
export LD_PRELOAD="./libtcmalloc.so"
ldd shmarena
sleep 2
./shmarena


