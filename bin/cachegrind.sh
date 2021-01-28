#!/bin/bash
valgrind --tool=cachegrind --trace-children=yes --branch-sim=yes ./shmarena
