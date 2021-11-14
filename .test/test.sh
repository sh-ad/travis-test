#!/bin/bash

TEST_FILE="$(realpath -e ./4200.txt)"

CC=clang
CXX=clang++
cp Makefile.soft ../Makefile
cd ..
make

RUN_COUNT=1
EXPECTED_RESULT=4200
CONSUMERS_THRESHOLD=100
CONSUMERS_STEP=9
SLEEP_THRESHOLD=0
CYCLE_COUNT=100

result="$(cat "${TEST_FILE}" | timeout 10s ./posix 1 2)"

for ((cycle=0; cycle<=${CYCLE_COUNT};cycle+=1)); do
  for ((consumers_amount=1; consumers_amount<=${CONSUMERS_THRESHOLD};consumers_amount+=${CONSUMERS_STEP})); do
      echo "Running with parameters: consumers amount = $consumers_amount, sleep threshold = ${SLEEP_THRESHOLD}" > log.txt
      result="$(cat "${TEST_FILE}" | timeout 10s ./posix "$consumers_amount" ${SLEEP_THRESHOLD})"
      status=$? 
      if [ $status -ne 0 ]; then
          echo "test failed with wrong error code, got $status (consumers amount = $consumers_amount, sleep threshold = ${SLEEP_THRESHOLD})"
          exit $status
      fi

      if ! [[ $result =~ ^[+-]?[0-9]+$ ]]; then
          echo "Invalid output: ${result}"
          exit 1
      fi
  
      if (( "$result" + 0 != "${EXPECTED_RESULT}" )); then
          echo "test failed with wrong summ; expected ${EXPECTED_RESULT}, got ${result} (consumers amount = $consumers_amount, sleep threshold = ${SLEEP_THRESHOLD})"
          exit 1
      fi
      
      echo "Running small test with parameters: consumers amount = $consumers_amount, sleep threshold = ${SLEEP_THRESHOLD}" > log.txt
      result=$(echo "1" | timeout 10s ./posix "$consumers_amount" ${SLEEP_THRESHOLD})
      status=$? 
  
      if [ $status -ne 0 ]; then
          echo "Small test failed with wrong error code, got $status (consumers amount = $consumers_amount, sleep threshold = ${SLEEP_THRESHOLD})"
          exit $status
      fi

      if ! [[ $result =~ ^[+-]?[0-9]+$ ]]; then
          echo "Invalid output: ${result}"
          exit 1
      fi
  
      if [ "$result" -ne "1" ]; then
          echo "Small test failed with wrong summ; expected ${EXPECTED_RESULT}, got ${result} (consumers amount = $consumers_amount, sleep threshold = ${SLEEP_THRESHOLD})"
          exit 1
      fi
  done
  
  echo "cycle number ${cycle} passed"
done

echo "all tests passed"


