#!/bin/bash
valgrind --tool=callgrind --trace-children=yes --cache-sim=yes --branch-sim=yes ./shmarena
