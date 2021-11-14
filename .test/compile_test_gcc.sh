#!/bin/bash

set -e

CC=gcc
CXX=g++
cp Makefile.hard ../Makefile
cd ..
if make; then
    echo "OK"
    git reset --hard HEAD
    git clean -f
    exit 0
else
    echo "FAIL"
    git reset --hard HEAD
    git clean -f
    exit 1
fi
