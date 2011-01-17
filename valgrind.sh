#!/bin/bash
set -x
valgrind -v --leak-check=full --show-reachable=yes --log-file=valgrind.log ./src/mplayerd -fg -d -p 7400
