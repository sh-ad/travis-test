#!/bin/bash

set -e
CC=clang
CXX=clang++
cp Makefile.soft ../Makefile
cd ..
echo "Step 1. Build"
make

echo "Step 2. Run test"
echo "echo '2 1 3 5 4 6 7 8 9' | timeout 10s ./posix 2 15"
OUT=$(echo '2 1 3 5 4 6 7 8 9' | timeout 10s ./posix 2 15)

RC=$?

git reset --hard HEAD
git clean -f

if [[ ${RC} != 0 ]]; then
    echo "FAIL! Return code: ${RC}"
    echo 1
fi

if (( ${OUT} == 45 )); then
    echo "OK"
    exit 0
else
    echo "FAIL! Out must be 45 but return ${OUT}" 
    exit 1
fi
