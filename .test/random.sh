#!/bin/bash

set -e
CC=clang
CXX=clang++

for file in $PWD; do
  echo "${file##*/}"
done
cp Makefile.soft */Makefile
cd */

echo $PWD
echo "Step 1. Build"
make

VALUES="$((( RANDOM )))"
for i in {1..100}; do
    VALUES+=" $((( RANDOM )))"
done

SUM=$(echo "$VALUES" | sed 's/ /+/g' | bc)

echo "Step 2. Run test"
echo "echo '${VALUES}' | timeout 10s ./posix 3 150"
OUT=$(echo "${VALUES}" | timeout 10s ./posix 3 150)

RC=$?

git reset --hard HEAD
git clean -f

if [[ ${RC} != 0 ]]; then
    echo "FAIL! Return code: ${RC}"
    echo 1
fi

if (( ${OUT} == ${SUM} )); then
    echo "OK"
    exit 0
else
    echo "FAIL! Out must be ${SUM} but return ${OUT}" 
    exit 1
fi
