#!/bin/bash
export LANG=C
export LC_CTYPE=C
export LC_ALL=C

cat src/zperf_counter.h  |sed `echo -e 's/\xEF\xBB\xBF//'` > src/zperf.h.bak
cat src/zperf_serialize.h  |sed `echo -e 's/\xEF\xBB\xBF//'` > src/zperf.h.bak
cat src/zperf_record.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> src/zperf.h.bak
cat src/zperf.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> src/zperf.h.bak

cat src/zperf.h.bak | sed '/#include.*zperf_/d' > src/zperf.h.merge
rm src/zperf.h.bak
