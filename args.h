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

// Command line argument processing

#ifndef _ARGS_H
#define _ARGS_H

typedef enum {
    OPT_BOOLEAN,
    OPT_REQUIRED_ARG,
    OPT_OPTIONAL_ARG,
} option_kind;

typedef struct {
    const char *name;
    const char *help_text;
    option_kind kind;
    int id;
} option_info;

#define END_OF_OPTIONS (option_info){0}

typedef struct {
    option_info *options;
    int ignore_options;
    int index;
} arg_parser;

typedef struct {
    const char *value;
    int id;
} arg_info;

// Error return values of arg_parser_next
#define ARGS_END                   -1
#define ARGS_OPTION_INVALID        -2
#define ARGS_OPTION_MISSING_ARG    -3
#define ARGS_OPTION_UNEXPECTED_ARG -4

void arg_parser_init(arg_parser *p, option_info *options);

int arg_parser_next(arg_parser *p, arg_info *arg, int argc, char *argv[]);

#endif // _ARGS_H
//------------------------------------------------------------------------------
#define ARGS_IMPLEMENTATION
#ifdef ARGS_IMPLEMENTATION

#include <stdlib.h>

void arg_parser_init(arg_parser *p, option_info *options) {
    p->options = options;
    p->ignore_options = 0;
    p->index = 1;
}

int _arg_parser_find_option(arg_parser *p, int *opt_ind, const char **arg_value,
        const char *name, int short_option) {
    int i, found_it = 0;
    *arg_value = NULL;
    name += short_option ? 1 : 2;
    for(int opt_i = 0; p->options[opt_i].name != NULL; ++opt_i) {
        found_it = 1;
        option_info opt = p->options[opt_i];
        if(!short_option)
            for(i = 0; name[i] != '\0' && name[i] != '='; ++i) {
                if(opt.name[i] != name[i])
                    found_it = 0;
            }
        else found_it = (opt.name[0] == name[0]);
        if(!found_it) continue;
        if(name[i] == '=')
            *arg_value = &name[i + 1];
        *opt_ind = opt_i; // found it!
        return 0;
    }
    // Didn't find it
    return ARGS_OPTION_INVALID;
}

int _arg_parser_got_argument(arg_parser *p, arg_info *arg,
        option_info option, const char *arg_value) {
    arg->value = arg_value;
    arg->id = option.id;
    if(arg_value != NULL)
        return (option.kind != OPT_BOOLEAN)
            ? 0 : ARGS_OPTION_UNEXPECTED_ARG;
    else return (option.kind != OPT_BOOLEAN)
            ? ARGS_OPTION_MISSING_ARG : 0;
}

int arg_parser_next(arg_parser *p, arg_info *arg, int argc, char *argv[]) {
start:
    int short_option = 1;
    int opt_i = -1;
    int status = 0;
    const char *arg_value;
    if(p->index >= argc) return ARGS_END;
    if(p->ignore_options || argv[p->index][0] != '-') {
        // Not an option, or it's explicity ignored
        arg->value = argv[p->index++];
        arg->id = 0;
        return 0;
    }
    if(argv[p->index][1] == '-') {
        if(argv[p->index][2] == '\0') {
            // -- stops option processing
            p->ignore_options = 1;
            p->index += 1;
            goto start;
        }
        short_option = 0;
    }
    // Which option is it?
    status = _arg_parser_find_option(p, &opt_i, &arg_value,
            argv[p->index], short_option);
    if(status != 0) {
        p->index += 1;
        return status;
    }
    option_info option = p->options[opt_i];
    if(arg_value != NULL) {
        p->index += 1;
        return _arg_parser_got_argument(p, arg, option, arg_value);
    }
    // Guess the argument must be in the next position of the array
    p->index += 1;
    if(option.kind == OPT_BOOLEAN) goto no_argument_needed;
    if(p->index >= argc
            || (argv[p->index][0] == '-' && !p->ignore_options))
        // It's not there
        return _arg_parser_got_argument(p, arg, option, NULL);
    arg_value = argv[p->index++];
no_argument_needed:
    return _arg_parser_got_argument(p, arg, option, arg_value);
}

#endif // ARGS_IMPLEMENTATION
