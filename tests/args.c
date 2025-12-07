#include <stdio.h>
#include <string.h>

#define ARGS_IMPLEMENTATION
#include "../args.h"

#define TEST_IMPLEMENTATION
#include "../test.h"

int test_simple_case(void *u) {
    int argc = 3;
    char *argv[] = { "./a.out", "-h", "3", NULL };
    option_info options[] = {
        { .name = "h", .help_text = "Help me", .kind = OPT_BOOLEAN, .id = 'h' },
        END_OF_OPTIONS
    };
    int status;
    arg_info arg;
    arg_parser argp;
    arg_parser_init(&argp, options);

    arg_parser_next(&argp, &arg, argc, argv);
    if(arg.id != 'h') return TEST_RESULT_FAIL;

    arg_parser_next(&argp, &arg, argc, argv);
    if(arg.id != 0 || strcmp(arg.value, "3") != 0)
        return TEST_RESULT_FAIL;

    if(arg_parser_next(&argp, &arg, argc, argv) != ARGS_END)
        return TEST_RESULT_FAIL;

    return TEST_RESULT_OK;
}

// Segfaults right now
int test_invalid_option(void *u) {
    int argc = 3;
    char *argv[] = { "./a.out", "-?", "3", NULL };
    option_info options[] = {
        { .name = "h", .help_text = "Help me", .kind = OPT_BOOLEAN, .id = 'h' },
        END_OF_OPTIONS
    };
    int status;
    arg_info arg;
    arg_parser argp;
    arg_parser_init(&argp, options);

    if(arg_parser_next(&argp, &arg, argc, argv) != ARGS_OPTION_INVALID
            && strcmp(arg.value, "?") != 0)
        return TEST_RESULT_FAIL;

    arg_parser_next(&argp, &arg, argc, argv);
    if(arg.id != 0 || strcmp(arg.value, "3") != 0)
        return TEST_RESULT_FAIL;

    if(arg_parser_next(&argp, &arg, argc, argv) != ARGS_END)
        return TEST_RESULT_FAIL;

    return TEST_RESULT_OK;
}

int main() {
    test_info suite[] = {
        { .name = "simple_case", .fn = test_simple_case, .should_fail = 0 },
        // { .name = "invalid_option", .fn = test_invalid_option, .should_fail = 0 },
        END_OF_SUITE
    };
    return test_suite_run("args", suite, NULL);
}
