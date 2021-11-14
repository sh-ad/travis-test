#!/bin/bash

set -e
CC=clang
CXX=clang++
cp Makefile.soft ../Makefile
cd ..
echo "Step 1. Build"
make

echo "Step 2. Run test"
echo "echo '1 2' | timeout 10s ./posix 2 0"
OUT=$(echo '1 2' | timeout 10s ./posix 2 0)

RC=$?

git checkout Makefile

if [[ ${RC} != 0 ]]; then
    echo "FAIL! Return code: ${RC}"
    echo 1
fi

if (( ${OUT} == 3 )); then
    echo "OK"
    exit 0
else
    echo "FAIL! Out must be 3 but return ${OUT}" 
    exit 1
fi
