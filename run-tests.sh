#!/bin/sh
# Copyright 2025 Eduardo Antunes dos Santos Vieira
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#---------------------------------------------------
# run-tests.sh: runs all tests in the tests folder |
# Author: Eduardo Antunes dos Santos Vieira        |
# Creation date: 2025-12-07                        |
# License: Apache 2.0                              |
# Usage: ./run-tests.sh                            |
#---------------------------------------------------

# Note: you could also use meson test for this, the exit values used by test.h
# are compatible with its testing infraestructure. For simpler projects though,
# you may find this script to be enough

first=1
for test_suite in tests/*; do
  if [ $first -eq 1 ]; then
    first=0
  else
    echo
  fi
  gcc "$test_suite" -o a.out
  ./a.out
done
[ -f a.out ] && rm a.out
