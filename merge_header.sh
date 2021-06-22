#!/bin/bash
export LANG=C
export LC_CTYPE=C
export LC_ALL=C

cat src/zprof_counter.h  |sed `echo -e 's/\xEF\xBB\xBF//'` > src/zprof.h.bak
cat src/zprof_serialize.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> src/zprof.h.bak
cat src/zprof_record.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> src/zprof.h.bak
cat src/zprof.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> src/zprof.h.bak

cat src/zprof.h.bak | sed '/#include.*zprof_/d' > src/zprof.h.merge
rm src/zprof.h.bak
