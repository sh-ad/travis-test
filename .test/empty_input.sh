#!/bin/bash

set -e
CC=clang
CXX=clang++
cp Makefile.soft ../Makefile
cd ..
make

RES=$(echo '' | timeout 10s ./posix 10 10)

if [[ $? != 0 ]]; then
    git reset --hard HEAD
    git clean -f
    echo "FAIL"
    echo 1
fi


if (( $RES == 0 )); then
    git reset --hard HEAD
    git clean -f
    echo "OK"
    exit 0
else
    git reset --hard HEAD
    git clean -f
    echo "FAIL must be 0 but return " $RES
    exit 1
fi

set -e
CC=clang
CXX=clang++
cp Makefile.soft ../Makefile
cd ..
echo "Step 1. Build"
make

echo "Step 2. Run test"
echo "echo '' | timeout 10s ./posix 10 10"
OUT=$(echo '' | timeout 10s ./posix 10 10)

RC=$?

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
