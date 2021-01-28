#!/bin/bash
if command -v lcov >/dev/null 2>&1; then 
  echo 'exists lcov' 
else 
  echo 'no exists lcov'
  exit 2 
fi

bcc=build-coverage
if [ ! -d "./$bcc" ]; then
  mkdir $bcc
fi
cd $bcc
cmake -DENABLE_GCOV=TRUE .. 
make -j4
cd ..
cd bin
sh tcmalloc.sh
#./shmarena
cd ..
pwd

#lcov --zerocounters --directory ./
lcov --capture --directory ./ --output-file ./bin/coverage.info
genhtml --highlight --legend --output-directory ./bin/report ./bin/coverage.info

