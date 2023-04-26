#!/bin/bash
export LANG=C
export LC_CTYPE=C
export LC_ALL=C

echo "#ifdef __GNUG__" > zprof.h.bak
echo "#pragma GCC push_options" >> zprof.h.bak
echo "#pragma GCC optimize (\"O2\")" >> zprof.h.bak
echo "#endif" >> zprof.h.bak
cat src/zprof_counter.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> zprof.h.bak
cat src/zprof_serialize.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> zprof.h.bak
cat src/zprof_record.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> zprof.h.bak
cat src/zprof.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> zprof.h.bak
echo "#ifdef __GNUG__" >> zprof.h.bak
echo "#pragma GCC pop_options" >> zprof.h.bak
echo "#endif" >> zprof.h.bak


mv zprof.h.bak  ./dist/zprof.h
cp README.md ./dist/README.md 
cp COPYRIGHT ./dist/LICENSE 

version=`date +"release zprof version date:%Y-%m-%d %H:%M:%S"`
echo $version > ./dist/VERSION 
echo "" >> ./dist/VERSION 
echo "git log:" >> ./dist/VERSION 
git log -1 --stat >> ./dist/VERSION 