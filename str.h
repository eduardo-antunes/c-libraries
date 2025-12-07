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

// Yet another string library for C

#ifndef _STR_H
#define _STR_H

#include <ctype.h>
#include <stddef.h>
typedef long long isize;

typedef struct {
    char *text;
    isize length, capacity;
} string;

typedef struct {
    const char *text;
    isize length;
} string_view;

#define string_foreach(ch, s) \
    ch = ((s)->length > 0) ? (s)->text[0] : 0; \
    for(isize _i = 0; _i < (s)->length; ch = (s)->text[++_i])

#define string_reverse_foreach(ch, s) \
    ch = ((s)->length > 0) ? (s)->text[(s)->length - 1] : 0; \
    for(isize _i = (s)->length - 1; _i >= 0; ch = (s)->text[--_i])

#define string_foreach_ptr(ptr, s) \
    ptr = (s)->text; \
    for(isize _i = 0; _i < (s)->length; ptr = &(s)->text[++_i])

#define string_reverse_foreach_ptr(ptr, s) \
    ptr = ((s->length) > 0) ? &(s)->text[(s)->length - 1] : NULL; \
    for(isize _i = (s)->length - 1; _i >= 0; ptr = &(s)->text[--_i])

void string_init(string *s);
void string_init_with_capacity(string *s, isize capacity);

void string_from_view(string *s, string_view sv);
void string_from_cstr(string *s, const char *cstr);
string_view string_view_from_cstr(const char *cstr);
string_view string_view_of(const string *s);

void string_push(string *s, char ch);
void string_concat(string *to, string_view sv);
void string_del(string *s, isize i);

string_view string_view_slice_from(string_view sv, isize begin);
string_view string_view_slice(string_view sv, isize begin, isize end);
string_view string_view_trim(string_view sv);

int string_view_eq(string_view sv1, string_view sv2);

#endif // _STR_H
//------------------------------------------------------------------------------
#ifdef STR_IMPLEMENTATION

#include <string.h>

#ifndef STR_BASE_SIZE
#define STR_BASE_SIZE 32
#endif // STR_BASE_SIZE

#ifndef string_alloc
#include <stdlib.h>
#define string_alloc(ptr, n) \
    (char*)realloc((ptr), (n) * sizeof(char));
#endif // string_alloc

void string_init(string *s) {
    s->capacity = s->length = 0;
    s->text = NULL;
}

void string_init_with_capacity(string *s, isize capacity) {
    string_init(s);
    if(capacity <= 0) return;
    s->text = string_alloc(NULL, capacity);
    if(s->text == NULL) return; // failed
    s->capacity = capacity;
    s->text[0] = '\0';
}

//------------------------------------------------------------------------------

static void _string_grow(string *s, isize min_capacity) {
    if(s->capacity == 0 && min_capacity > 0)
        s->capacity = STR_BASE_SIZE;
    while(s->capacity < min_capacity)
        s->capacity *= 2;
    s->text = string_alloc(s->text, s->capacity);
}

void string_from_cstr(string *s, const char *cstr) {
    string_init_with_capacity(s, strlen(cstr) + 1);
    for(isize i = 0; cstr[i] != '\0'; ++i) {
        s->text[s->length] = cstr[i];
        s->length += 1;
    }
    s->text[s->length] = '\0';
}

void string_from_view(string *s, string_view sv) {
    char *ptr;
    string_init_with_capacity(s, sv.length + 1);
    string_foreach_ptr(ptr, s)
        *ptr = sv.text[_i];
    s->text[s->length] = '\0';
}

string_view string_view_from_cstr(const char *cstr) {
    return (string_view) { .text = cstr, .length = (isize)strlen(cstr) };
}

string_view string_view_of(const string *s) {
    return (string_view) { .text = s->text, .length = s->length };
}

//------------------------------------------------------------------------------

void string_push(string *s, char ch) {
    _string_grow(s, s->length + 2);
    s->text[s->length++] = ch;
    s->text[s->length] = '\0';
}

void string_concat(string *s, string_view sv) {
    char ch;
    _string_grow(s, s->length + sv.length + 1);
    string_foreach(ch, &sv) {
        s->text[s->length] = ch;
        s->length += 1;
    }
    s->text[s->length] = '\0';
}

void string_del(string *s, isize i) {
    if(i < 0) i = s->length + i;
    for(isize j = i; j < s->length; ++j)
        s->text[j] = s->text[j + 1];
    s->length -= 1;
}

//------------------------------------------------------------------------------

string_view string_view_slice(string_view sv, isize begin, isize end) {
    if(begin < 0) begin = sv.length + begin;
    if(end < 0) end = sv.length + end;
    if(end < begin) return (string_view) {0};
    return (string_view) {
        .text = &sv.text[begin],
        .length = end - begin
    };
}

string_view string_view_slice_from(string_view sv, isize begin) {
    if(begin < 0) begin = sv.length + begin;
    return (string_view) {
        .text = &sv.text[begin],
        .length = sv.length - begin
    };
}

string_view string_view_trim(string_view sv) {
    char ch;
    string_view res = {0};
    if(sv.text == NULL || sv.length == 0) return res;
    res.text = sv.text;
    res.length = sv.length;
    string_foreach(ch, &sv) {
        if(!isspace(ch)) break;
        res.text += 1;
        res.length -= 1;
    }
    if(*res.text == '\0') return res;
    string_reverse_foreach(ch, &sv) {
        if(!isspace(ch)) break;
        res.length -= 1;
    }
    return res;
}

//------------------------------------------------------------------------------


int string_view_eq(string_view sv1, string_view sv2) {
    if(sv1.length != sv2.length) return 0;
    return memcmp(sv1.text, sv2.text, sv1.length) == 0;
}

#undef string_alloc
#undef STR_BASE_SIZE
#endif // STR_IMPLEMENTATION
// vim: set ft=c :
