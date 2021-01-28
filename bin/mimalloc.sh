#!/bin/bash
export LD_PRELOAD="./libmimalloc.so"
ldd shmarena
sleep 2
./shmarena


