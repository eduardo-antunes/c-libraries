#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define STR_IMPLEMENTATION
#include "../str.h"

#define TEST_IMPLEMENTATION
#include "../test.h"

int test_concat(void *u) {
    string msg;
    string_init_with_capacity(&msg, 2);
    string_concat(&msg, string_view_from_cstr("Hello, "));

    string world;
    string_from_cstr(&world, "world");
    string_concat(&msg, string_view_of(&world));

    string ending;
    string_from_cstr(&ending, "! All good?");
    string_view end_view = string_view_of(&ending);
    string_concat(&msg, end_view);

    if(strcmp(msg.text, "Hello, world! All good?") == 0)
        return TEST_RESULT_OK;
    return TEST_RESULT_FAIL;
}

int test_string_build(void *u) {
    string builder;
    string_init_with_capacity(&builder, 2);
    const char *msg = "Hello, world! All good with you?";
    for(int i = 0; msg[i] != '\0'; ++i)
        // Obviously inefficient, this is just a "stress" test
        // Use the string_concat function instead
        string_push(&builder, msg[i]);
    if(strcmp(builder.text, msg) == 0)
        return TEST_RESULT_OK;
    return TEST_RESULT_FAIL;
}

int test_trim(void *u) {
    string_view hello = string_view_from_cstr("  hello  ");
    string_view res = string_view_trim(hello);
    if(string_view_eq(res, string_view_from_cstr("hello")))
        return TEST_RESULT_OK;
    return TEST_RESULT_FAIL;
}

int test_slice(void *u) {
    string_view hello_world = string_view_from_cstr("Hello, World!");

    string_view hello = string_view_slice(hello_world, 0, 5);
    if(!string_view_eq(hello, string_view_from_cstr("Hello")))
        return TEST_RESULT_FAIL;

    string_view world = string_view_slice_from(hello_world, 7);
    if(!string_view_eq(world, string_view_from_cstr("World!")))
        return TEST_RESULT_FAIL;

    string_view no_punct = string_view_slice(hello_world, 0, -1);
    if(!string_view_eq(no_punct, string_view_from_cstr("Hello, World")))
        return TEST_RESULT_FAIL;

    return TEST_RESULT_OK;
}

int main() {
    test_info suite[] = {
        { .name = "concat", .fn = test_concat, .should_fail = 0 },
        { .name = "string_build", .fn = test_string_build, .should_fail = 0 },
        { .name = "trim", .fn = test_trim, .should_fail = 0 },
        { .name = "slice", .fn = test_slice, .should_fail = 0 },
        END_OF_SUITE
    };
    return test_suite_run("str", suite, NULL);
}
