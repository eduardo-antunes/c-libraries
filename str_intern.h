/*
 * Copyright 2025 Eduardo Antunes dos Santos Vieira
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

// Interning a string means mapping it to a unique copy, stored at a fixed
// address in memory. The address thus becomes a representation of its contents,
// and you can check string equality by address comparison - a fast operation.

// The functions in this header are all named under the string_arena_ and
// string_repo_ prefixes. The types HashFn, ReallocFn, StringView, StringArena,
// StringRegion and StringRepo are defined, as well as the SV_FMT and SV_ARG
// macros. Avoiding those prefixes and names in your own code should be enough
// to prevent any name clashes. (I could've prefixed everything, like the STB
// headers do, but that would be a joyless endeavour)

#ifndef STR_INTERN_H
#define STR_INTERN_H

#include <stddef.h>
#include <stdint.h>

// Wide pointer to a string
typedef struct {
    const char *text;
    size_t len; // should not include null byte
} StringView;

#define SV_FMT "%.*s"
#define SV_ARG(sv) (int)(sv).len,(sv).text

typedef uint64_t (*HashFn)(StringView);
typedef void* (*ReallocFn)(void *, size_t);

// Simple arena of immutable strings
typedef struct {
    ReallocFn realloc;
    struct string_region *first, *last;
    size_t total_capacity;
} StringArena;

// Repository of interned strings
typedef struct {
    HashFn hash;
    ReallocFn realloc;
    StringArena arena;
    struct hashed_view *contents;
    size_t count, capacity;
} StringRepo;

void string_repo_init(StringRepo *sr);
void string_repo_init_with(StringRepo *sr, HashFn hash, ReallocFn realloc);
void string_repo_free(StringRepo *sr);

// Interns a string, returning a view to its unique copy
StringView string_repo_get(StringRepo *sr, StringView sv);

// Fast comparison between two interned views
#define sv_interned_eq(sv1, sv2) ((sv1).text == (sv2).text)

#endif // STR_INTERN_H

// -----------------------------------------------------------------------------

#ifdef STR_INTERN_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

typedef struct string_region {
    struct string_region *next;
    size_t count, capacity;
    char contents[];
} StringRegion;

static StringRegion *string_arena_region_new(StringArena *ar, size_t cap) {
    StringRegion *r = (StringRegion *) ar->realloc(NULL, sizeof(StringRegion)
            + cap * sizeof(char) /* flexible array member */);
    // Assumes the returned pointer is non-NULL
    r->count = 0;
    r->capacity = cap;
    r->next = NULL;
    return r;
}

static void string_arena_init(StringArena *ar, ReallocFn realloc) {
    ar->realloc = realloc;
    ar->first = ar->last = NULL;
    ar->total_capacity = 0;
}

static void string_arena_free(StringArena *ar) {
    StringRegion *ptr = ar->first, *aux;
    while(ptr != NULL) {
        aux = ptr->next;
        ar->realloc(ptr, 0);
        ptr = aux;
    }
    string_arena_init(ar, ar->realloc);
}

#define INITIAL_CAP   256
#define GROWTH_FACTOR 2

// Copies the given string view into the arena
static StringView string_arena_copy(StringArena *ar, StringView sv) {
    // Check if any of the regions has enough space for sv
    StringRegion *reg = ar->first;
    while(reg != NULL) {
        if(reg->count + sv.len + 1 <= reg->capacity) break;
        reg = reg->next;
    }

    if(reg == NULL) {
        // None of the existing region had enough space. We allocate a new one.
        // If there are no regions, it is of size INITIAL_CAP. Otherwise, it has
        // a size such that the total capacity grows by the GROWTH_FACTOR
        size_t cap = (ar->total_capacity == 0) ? INITIAL_CAP
            : (GROWTH_FACTOR - 1) * ar->total_capacity;

        // There is one problem though: what if the normal growth can't fit the
        // provided string? In this case, the exceptional behavior is to make
        // the whole region the size of the string, to guarantee it fits
        if(cap < sv.len + 1) cap = sv.len + 1;

        reg = string_arena_region_new(ar, cap);
        if(ar->last != NULL) {
            ar->last->next = reg;
            ar->last = reg;
        } else ar->first = ar->last = reg;
        ar->total_capacity += cap;
    }

    // Here, reg definitely points to a region with enough free space
    char *ptr = &reg->contents[reg->count];
    memcpy(ptr, sv.text, sv.len);
    reg->count += sv.len + 1;
    ptr[sv.len] = 0; // null byte for compatibility with libc
    sv.text = ptr;
    return sv;
}

#undef INITIAL_CAP
#undef GROWTH_FACTOR

// -----------------------------------------------------------------------------

typedef struct hashed_view {
    const char *text;
    uint64_t hash;
    size_t len;
} HashedView;

static HashedView *string_repo_array_new(StringRepo *sr, size_t cap) {
    HashedView *arr = (HashedView*) sr->realloc(NULL, cap * sizeof(HashedView));
    memset(arr, 0, cap * sizeof(HashedView));
    return arr;
}

// Finds the index where a given hashed view should be at
// If it's been inserted already, it will be there
static size_t string_repo_array_find(HashedView *arr, size_t cap,
        HashedView hv) {
    // The hashset upon which the repo is founded is based on open addressing
    // with linear probing, the simplest scheme for this sort of thing. NOTE
    // the cap is guaranteed to always be a power of two
    size_t ind = (size_t)(hv.hash & (uint64_t)(cap - 1));
    while(arr[ind].text != NULL) {
        if(hv.len == arr[ind].len
                && memcmp(hv.text, arr[ind].text, hv.len) == 0)
            return ind; // the string is there already
        ind += 1;
        if(ind >= cap) ind = 0;
    }
    return ind; // the string should be here
}

// -----------------------------------------------------------------------------

#define INITIAL_CAP   128
#define GROWTH_FACTOR 2

static void string_repo_alloc(StringRepo *sr) {
    sr->contents = string_repo_array_new(sr, INITIAL_CAP);
    sr->capacity = INITIAL_CAP;
}

// Expands the memory capacity of the repo
static void string_repo_expand(StringRepo *sr) {
    size_t cap = sr->capacity * GROWTH_FACTOR;
    HashedView *arr = string_repo_array_new(sr, cap);

    // We need to reinsert each of the old keys in the new array
    for(size_t i = 0; i < sr->capacity; ++i) {
        if(sr->contents[i].text == NULL) continue;
        size_t new_ind = string_repo_array_find(arr, cap, sr->contents[i]);
        arr[new_ind] = sr->contents[i];
    }
    sr->realloc(sr->contents, 0); // free the old array
    sr->contents = arr;
    sr->capacity = cap;
}

// Inserts a new string into the repo
static void string_repo_insert(StringRepo *sr, HashedView hv, size_t ind) {
    // Do we need more memory? The load factor should be limited to 0.5
    if(sr->count + 1 >= sr->capacity / 2) {
        string_repo_expand(sr);
        // We have to recalculate the index after an expansion
        ind = string_repo_array_find(sr->contents, sr->capacity, hv);
    }
    sr->count += 1;
    sr->contents[ind] = hv;
}

#undef INITIAL_CAP
#undef GROWTH_FACTOR

// -----------------------------------------------------------------------------

// The default hash function implements the classic 64-bit FNV-1a algorithm

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME  1099511628211UL

static uint64_t string_repo_default_hash(StringView sv) {
    uint64_t h = FNV_OFFSET;
    for(size_t i = 0; i < sv.len; ++i) {
        h ^= (uint64_t)(unsigned char)(sv.text[i]);
        h *= FNV_PRIME;
    }
    return h;
}

#undef FNV_OFFSET
#undef FNV_PRIME

// Default realloc function, based on stdlib. Exits on NULL pointer
static void *string_repo_default_realloc(void *ptr, size_t size) {
    if(size == 0) {
        free(ptr);
        return NULL;
    }
    void *new_ptr = realloc(ptr, size);
    if(new_ptr == NULL) exit(64); // out of memory
    return new_ptr;
}

// -----------------------------------------------------------------------------

void string_repo_init(StringRepo *sr) {
    string_repo_init_with(sr, string_repo_default_hash,
            string_repo_default_realloc);
}

void string_repo_init_with(StringRepo *sr, HashFn hash, ReallocFn realloc) {
    sr->hash = hash;
    sr->realloc = realloc;
    string_arena_init(&sr->arena, realloc);
    sr->contents = NULL;
    sr->count = sr->capacity = 0;
}

void string_repo_free(StringRepo *sr) {
    sr->realloc(sr->contents, 0);
    string_arena_free(&sr->arena);
    string_repo_init_with(sr, sr->hash, sr->realloc);
}

StringView string_repo_get(StringRepo *sr, StringView sv) {
    if(sr->contents == NULL) string_repo_alloc(sr);
    // If sv is in the repository already, we're done
    uint64_t hash = sr->hash(sv);
    HashedView hv = { .text = sv.text, .hash = hash, .len = sv.len };
    size_t ind = string_repo_array_find(sr->contents, sr->capacity, hv);
    if(sr->contents[ind].text != NULL)
        return (StringView){
            .text = sr->contents[ind].text,
            .len = sr->contents[ind].len,
        };

    // Otherwise, copy its contents into the arena and insert it
    sv = string_arena_copy(&sr->arena, sv);
    hv.text = sv.text;
    string_repo_insert(sr, hv, ind);
    return sv;
}

#endif // STR_INTERN_IMPLEMENTATION
// vim:set ft=c:
