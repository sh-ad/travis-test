#!/bin/bash

set -e
CC=clang
CXX=clang++
cp Makefile.soft ../Makefile
cd ..
echo "Step 1. Build"
make
set +e

echo "Step 2. Run test"
echo "echo '1' | timeout 10s ./posix"
RES=$(echo '1' | timeout 10s ./posix)

RC=$?

git reset --hard HEAD
git clean -f

if [[ ${RC} == 0 ]]; then
    echo "FAIL! Return code: 0"
    exit 1
else
    echo "OK"
    exit 0
fi
