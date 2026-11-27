#!/bin/bash

./bin/big-int 2>&1 | diff - test/big-int/big-int.expected
