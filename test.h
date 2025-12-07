/*
Copyright 2025 Eduardo Antunes dos Santos Vieira

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// Simple library for running tests in C

#ifndef _TEST_H
#define _TEST_H

#define TEST_RESULT_OK         0
#define TEST_RESULT_FAIL       1
#define TEST_RESULT_SKIP       77
#define TEST_RESULT_SKIP_SUITE 78
#define TEST_RESULT_HARD_FAIL  99

typedef int (*test_fn)(void *userdata);

typedef struct {
    const char *name;
    test_fn fn;
    int should_fail;
} test_info;

#define END_OF_SUITE { .name = NULL, .fn = NULL, .should_fail = 0 }

int test_suite_run(const char *name, test_info suite[], void *userdata);

#endif // _TEST_H
//------------------------------------------------------------------------------
#ifdef TEST_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>

#define test_log(...) fprintf(stderr, __VA_ARGS__)

#define esc             "\033"
#define color_reset   esc "[0m"
#define color_green   esc "[0;32m"
#define color_magenta esc "[0;35m"
#define color_red     esc "[0;31m"

#define test_log_ok()         test_log(color_green "ok" color_reset)
#define test_log_skip()       test_log(color_magenta "skip" color_reset)
#define test_log_skip_suite() test_log(color_magenta "skip_suite" color_reset)
#define test_log_fail()       test_log(color_red "fail" color_reset)

int test_suite_run(const char *name, test_info suite[], void *userdata) {
    int skip_suite = 0;
    int suite_status = TEST_RESULT_OK;
    int ok = 0, skip = 0, fail = 0;
    test_log("Running test suite %s...\n", name);
    for(int i = 0; suite[i].fn != NULL; ++i) {
        if(skip_suite) {
            skip += 1;
            continue;
        }
        test_info current_test = suite[i];
        test_log("- %s: ", current_test.name);
        int status = current_test.fn(userdata);
        switch(status) {
            case TEST_RESULT_OK:
                test_log_ok();
                ok += 1;
                break;
            case TEST_RESULT_SKIP:
                test_log_skip();
                skip += 1;
                break;
            case TEST_RESULT_SKIP_SUITE:
                suite_status = TEST_RESULT_SKIP;
                test_log_skip_suite();
                skip_suite = 1;
                skip += 1;
                break;
            case TEST_RESULT_HARD_FAIL:
                suite_status = TEST_RESULT_FAIL;
                test_log_fail();
                fail += 1;
                break;
            default:
                if(current_test.should_fail) {
                    status = TEST_RESULT_OK;
                    test_log_ok();
                    ok += 1;
                } else {
                    suite_status = TEST_RESULT_FAIL;
                    status = TEST_RESULT_FAIL;
                    test_log_fail();
                    fail += 1;
                }
                break;
        }
        test_log("\n");
    }
    test_log("%d passed, %d skipped, %d failed\n", ok, skip, fail);
    return suite_status;
}

#undef test_log
#undef esc
#undef color_reset
#undef color_green
#undef color_magenta
#undef color_red
#undef test_log_ok
#undef test_log_skip
#undef test_log_skip_suite
#undef test_log_fail
#endif // TEST_IMPLEMENTATION
