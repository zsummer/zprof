#!/bin/bash
export LANG=C
export LC_CTYPE=C
export LC_ALL=C

echo "#ifdef __GNUG__" > src/zprof.h.bak
echo "#pragma GCC push_options" >> src/zprof.h.bak
echo "#pragma GCC optimize (\"O2\")" >> src/zprof.h.bak
echo "#endif" >> src/zprof.h.bak
cat src/zprof_counter.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> src/zprof.h.bak
cat src/zprof_serialize.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> src/zprof.h.bak
cat src/zprof_record.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> src/zprof.h.bak
cat src/zprof.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> src/zprof.h.bak
echo "#ifdef __GNUG__" >> src/zprof.h.bak
echo "#pragma GCC pop_options" >> src/zprof.h.bak
echo "#endif" >> src/zprof.h.bak

cat src/zprof.h.bak | sed '/#include.*zprof_/d' > src/zprof.h.merge
rm src/zprof.h.bak
