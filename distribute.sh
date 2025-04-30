#!/bin/bash
export LANG=C
export LC_CTYPE=C
export LC_ALL=C

echo "#ifdef __GNUG__" > zprof.h.bak
echo "#pragma GCC push_options" >> zprof.h.bak
echo "#pragma GCC optimize (\"O2\")" >> zprof.h.bak
echo "#endif" >> zprof.h.bak
cat src/zprof_clock.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> zprof.h.bak
cat src/zprof_report.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> zprof.h.bak
cat src/zprof_record.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> zprof.h.bak
cat src/zprof.h  |sed `echo -e 's/\xEF\xBB\xBF//'` >> zprof.h.bak
echo "#ifdef __GNUG__" >> zprof.h.bak
echo "#pragma GCC pop_options" >> zprof.h.bak
echo "#endif" >> zprof.h.bak

cat zprof.h.bak | sed '/#include.*zprof_/d' > ./dist/include/zprof/zprof.h
rm zprof.h.bak

cp README.md ./dist/include/zprof/README.md 
cp COPYRIGHT ./dist/include/zprof/LICENSE 

last_sha1=`git rev-parse HEAD`
last_date=`git show --pretty=format:"%ci" | head -1`
last_diff=`git log -1 --stat `

last_dist_sha1=`git log -1 --stat ./src |grep -E "commit ([0-9a-f]*)" |grep -E -o "[0-9a-f]{10,}"`
last_dist_date=`git show $last_dist_sha1 --pretty=format:"%ci" | head -1`
last_dist_diff=`git log -1 --stat ./src`

echo ""
echo "[zprof last commit]:"
echo $last_sha1
echo $last_date
echo ""
echo "[zprof last diff]:"
echo $last_diff


echo ""
echo "[./src last commit]:"
echo $last_dist_sha1
echo $last_dist_date
echo ""
echo "[./src last diff]:"
echo $last_dist_diff

echo ""
echo "[write versions]"
echo "version:" > ./dist/include/zprof/GIT_VERSION
echo "last_sha1(./src)=$last_dist_sha1" >> ./dist/include/zprof/GIT_VERSION 
echo "last_date(./src)=$last_dist_date" >> ./dist/include/zprof/GIT_VERSION 
echo "" >> ./dist/include/zprof/GIT_VERSION 
echo "git log -1 --stat ./src:" >> ./dist/include/zprof/GIT_VERSION 
echo $last_dist_diff >> ./dist/include/zprof/GIT_VERSION
cat ./dist/include/zprof/GIT_VERSION

echo ""
echo "[write done]"
