#!/bin/bash

TEST_FAILED=0
INTERPRETER=../../cyarg/build/host-test/cyarg

$INTERPRETER int.ya | diff int_expect.txt - || TEST_FAILED=1
$INTERPRETER casts.ya | diff casts_expect.txt - || TEST_FAILED=1

# check the expected outut is simple to generate: iteration number + 5 zeros.
$INTERPRETER bc_expect.ya | diff bc_expect.txt - || TEST_FAILED=1

echo "tests so far $TEST_FAILED"

# benchmark, so just need to note the output, not compare it
$INTERPRETER perform.ya
$INTERPRETER perform-t.ya

echo "starting bc test"
# ask diff to supply 6 lines of context, so we see the iteration number for the diff
$INTERPRETER bc.ya | bc -lLS 0 | diff -C 6 bc_expect.txt - || TEST_FAILED=1

exit $TEST_FAILED