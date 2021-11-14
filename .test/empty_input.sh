#!/bin/bash
set -e
CC=clang
CXX=clang++

cp Makefile.soft Makefile
echo "Step 1. Build"
make

echo "Step 2. Run test"
echo "echo '' > in.txt && timeout 10s ./posix 10 10"

rm -f "in.txt"
echo '' > "in.txt"
OUT=$(timeout 10s ./posix 10 10)
RC=$?

rm -f "in.txt"

git reset --hard HEAD
git clean -f

if [[ ${RC} != 0 ]]; then
    echo "FAIL! Return code: ${RC}"
    echo 1
fi

if (( ${OUT} == 0 )); then
    echo "OK"
    exit 0
else
    echo "FAIL! Out must be 0 but return ${OUT}" 
    exit 1
fi