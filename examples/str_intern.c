// Just a very simple test of the string interning

#include <stdio.h>
#include <string.h>

#define STR_INTERN_IMPLEMENTATION
#include "../str_intern.h"

StringView intern(StringRepo *r, const char *text) {
    StringView sv = { .text = text, .len = strlen(text) };
    return string_repo_get(r, sv);
}

int main() {
    StringRepo r;
    string_repo_init(&r);

    StringView s1 = intern(&r, "In a hole in the ground there lived a hobbit");
    StringView s2 = intern(&r, "In a hole in the ground there lived a...");
    if(!sv_interned_eq(s1, s2)) printf("Ok\n");

    char buf[1024];
    StringView tigers = intern(&r, "three sad tigers");
    scanf("%[^\n]", buf); // try typing 'three sad tigers'
    StringView intern_buf = intern(&r, buf);
    if(sv_interned_eq(tigers, intern_buf))
        printf("Nice!\n");

    // Teste libc compatibility
    printf("String 1: %s\nString 2: %s\nIntern buf: %s\n",
            s1.text, s2.text, intern_buf.text);
    string_repo_free(&r);
    return 0;
}
