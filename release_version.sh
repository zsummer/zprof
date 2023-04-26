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

cat zprof.h.bak | sed '/#include.*zprof_/d' > ./dist/include/zprof/zprof.h
rm zprof.h.bak

cp README.md ./dist/include/zprof/README.md 
cp COPYRIGHT ./dist/include/zprof/LICENSE 

version=`date +"release zprof version date:%Y-%m-%d %H:%M:%S"`
echo $version > ./dist/include/zprof/VERSION 
echo "" >> ./dist/include/zprof/VERSION 
echo "git log:" >> ./dist/include/zprof/VERSION 
git log -1 --stat >> ./dist/include/zprof/VERSION 