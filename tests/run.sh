#!/bin/sh
# Host-side tests (needs gcc). Does not need CEdev or a calculator.
cd "$(dirname "$0")" && gcc -std=c99 -Wall -Wextra -Wno-unused-function -I stub \
  -o /tmp/metroce_tests test_all.c ../src/input.c ../src/area.c ../src/visible.c ../src/traffic.c ../src/city.c \
  && /tmp/metroce_tests
