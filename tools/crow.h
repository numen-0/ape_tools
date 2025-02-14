/*
# crow.h - v0.0.1 - Simple and Portable Argument Parser - by numen-0

 This is a single-header-file library providing utilities for argument parsing.

 For more tools, see:
 > https://github.com/numen-0/ape_tools

 To use this library, include it in c or c++ file:

    // do this def each traslation unit, that requires the lib
    #define CROW_POSITIONAL
    #define CROW_IMPLEMENTATION
    #include "crow.h"

    // or
    #define CROW_PARAMETRIC
    #define CROW_IMPLEMENTATION
    #include "crow.h"


## TABLE OF CONTENTS
- Documentation
    - Compile-time options
    - API Reference
    - Notes
- License
- Credits

## DOCUMENTATION

### COMPILE-TIME OPTIONS
- `CROW_STRIP_PREFIX` (TODO)

        If defined, removes the `CROW` prefix from all macros.

- `CROW_POSITIONAL`

        Enables **positional mode**, where arguments are passed by index.

- `CROW_PARAMETRIC` (TODO)

        Enables **parametric mode**, where arguments are accessed by flags,
        making them position-independent.


### API REFERENCE

- void crow_init(Crow_Parser* crow, const char* name, const char* desc);

        initializes a previously allocated `Crow_Parser*`.

- int crow_parse(Crow_Parser* crow, int argc, char* argv[]);

        parses arguments following the previously defined structure.
        returns 0 on success.

- void crow_close(Crow_Parser* crow);

        frees the internal structures allocated by the parser.

- void crow_get_by_name(Crow_Parser* crow, const char* name, void** out);

        searches for a parameter by `name` and stores its value in `out`.
        if not found, sets `out` to `NULL`.

#### POSITIONAL MODE

- `void crow_add_arg(Crow_Parser* crow, const char* name, crow_arg_t arg_t, bool
required, uintptr_t def_val, const char* desc);`

        Adds a **position-dependent** argument.

- `void crow_get_indx(Crow_Parser* crow, int indx, void** out);`

        Retrieves the argument at `indx` and stores its value in `out`.
        If `indx` is out of range, sets `out` to `NULL`.

- `#define crow_addArg(crow, name, type, required, def_val, desc)`

        Macro wrapper for `crow_add_arg`.

- `#define crow_addReqArg(crow, name, type, desc)`

        Macro wrapper for `crow_add_arg` with `required = true`.


#### PARAMETRIC MODE (WIP)

...

### NOTES:
 * the interfaces may change to improve usability.


## LICENSE

   This software is licensed under the GNU General Public License v3.0 or later.
   See the end of the file for detailed license information.

## CREDITS:
 * inspired by stb-style single-header libraries.
*/
#ifndef _CROW_H ///////////////////////////////////////////////////////////////
#    define _CROW_H

#    ifdef __cplusplus
extern "C" {
#    endif

#    if defined(CROW_POSITIONAL) && defined(CROW_PARAMETRIC) // sanity check
#        error "only one mode can be set"
#    elif !defined(CROW_POSITIONAL) && !defined(CROW_PARAMETRIC)
#        error "one mode must be set"
#    endif

#    if defined(CROW_PARAMETRIC)
#        error "not implemented!!!"
#    endif

#    ifndef __STDC_VERSION__
#        define __STDC_VERSION__ 199409L // Assume at least C90
#    endif
#    if __STDC_VERSION__ < 199901L
typedef unsigned long uintptr_t;         // Simple fallback for pre-C99
#    endif


#    ifndef __cplusplus
#        include <stdbool.h>
#    endif

typedef struct Crow_Parser_s Crow_Parser;

typedef enum {
    CROW_BOOL = 1,
    CROW_CHAR,
    CROW_SHORT,
    CROW_INT,
    CROW_LONG,
    CROW_LLONG,
    CROW_FLOAT,
    CROW_DOUBLE,
    CROW_STRING,
} crow_arg_t;

// HACK: to make the interface macro interface simple
#    define _CROW__bool   CROW_BOOL
#    define _CROW__char   CROW_CHAR
#    define _CROW__short  CROW_SHORT
#    define _CROW__int    CROW_INT
#    define _CROW__long   CROW_LONG
#    define _CROW__llong  CROW_LLONG
#    define _CROW__float  CROW_FLOAT
#    define _CROW__double CROW_DOUBLE
#    define _CROW__string CROW_STRING

static void crow_init(Crow_Parser* crow, const char* name, const char* desc);
static int crow_parse(Crow_Parser* crow, int argc, char* argv[]);
static void crow_close(Crow_Parser* crow);

static void crow_get_by_name(Crow_Parser* crow, const char* name, void** out);


#    ifdef CROW_POSITIONAL
#        include <stdint.h>

static void crow_add_arg(Crow_Parser* crow, const char* name, crow_arg_t arg_t,
    bool required, uintptr_t def_val, const char* desc);
static void crow_get_indx(Crow_Parser* crow, int indx, void** out);

#        define crow_addArg(crow, name, type, required, def_val, desc)         \
            crow_add_arg(                                                      \
                crow, name, _CROW__##type, required, (uintptr_t)def_val, desc)
#        define crow_addReqArg(crow, name, type, desc)             \
            crow_add_arg(crow, name, _CROW__##type, true, 0, desc)

#    endif // !CROW_POSITIONAL
#    ifdef CROW_PARAMETRIC

// NOTE: in the from 0-15 bits the user can define groups their arg groups.
// NOTE: -7 and 7 are not the same group
typedef enum {
    CROW_NONE,      // no rules
    CROW_REQUIRE,   // all in group are required
    CROW_EXCLUSIVE, // only one in the group can be passed
    CROW_INCLUSIVE, // at least one in the group needs to be passed
} crow_group_rule_t;

// do we need this?
// typedef bool (*Crow_Rule)(const void*);

static void crow_add_rule(Crow_Parser* crow, int group, crow_group_rule_t rule);
static void crow_add_arg(Crow_Parser* crow, char s_flag, char* l_flag,
    crow_arg_t arg_t, void* def_val, int group, const char* desc);

#        define crow_addArg(crow, sf, lf, type, def, group, desc)       \
            crow_add_arg(crow, sf, lt, _CROW__##type, def, group, desc)
#        define crow_addFlag(crow, sf, lf, group, desc)               \
            crow_add_arg(crow, sf, lt, CROW_BOOL, false, group, desc)

#    endif                     // !CROW_PARAMETRIC

#    ifdef CROW_IMPLEMENTATION ////////////////////////////////////////////////
#        undef CROW_IMPLEMENTATION

#        include <assert.h>
#        include <limits.h>
#        include <stdio.h>
#        include <stdlib.h>
#        include <string.h>

#        define crow_malloc  malloc
#        define crow_realloc realloc
#        define crow_free    free
#        define crow_assert  assert
#        define crow_ptr_assert(ptr)                        \
            (assert(ptr != NULL && "buy more ram :("), ptr)


#        define CROW__DA_INIT_CAP 64
#        define CROW__DA(type) \
            struct {           \
                int count;     \
                int capacity;  \
                type* items;   \
            }
#        define CROW__DAinit(da)                                           \
            do {                                                           \
                (da).count = 0;                                            \
                (da).capacity = CROW__DA_INIT_CAP;                         \
                (da).items = (__typeof__((da).items))crow_ptr_assert(      \
                    crow_malloc(sizeof(*(da).items) * CROW__DA_INIT_CAP)); \
            } while ( 0 )
#        define CROW__DAreset(da) (da).count = 0;
#        define CROW__DAfree(da)  crow_free((da).items)

#        define CROW__DAappend(da, item)                                  \
            do {                                                          \
                if ( (da).capacity <= (da).count ) {                      \
                    (da).capacity = (da).capacity * 2;                    \
                    (da).items = (__typeof__((da).items))crow_ptr_assert( \
                        crow_realloc((da).items, (da).capacity));         \
                }                                                         \
                (da).items[(da).count++] = item;                          \
            } while ( 0 )

typedef struct Crow_Arg_s Crow_Arg;

#        ifdef CROW_POSITIONAL ////////////////////////////////////////////////

#            define CROW_ERROR(msg, ...)                               \
                fprintf(stderr, "crow:error " msg "\n", ##__VA_ARGS__)
struct Crow_Parser_s {
    const char* name;
    const char* description;
    CROW__DA(Crow_Arg*) args;
};
struct Crow_Arg_s {
    const char* name;
    const char* desc;
    uintptr_t def_val;
    union {
        uintptr_t raw;
        void* ptr;
    } val;
    crow_arg_t arg_t;
    bool required;
};

void crow_init(Crow_Parser* crow, const char* name, const char* desc)
{
    crow->name = name;
    crow->description = desc;
    CROW__DAinit(crow->args);
}
static void crow__print_help(Crow_Parser* crow)
{
    printf("Usage: %s", crow->name);
    for ( int i = 0; i < crow->args.count; i++ ) {
        printf(" %s", crow->args.items[i]->name);
    }
    printf("\n");
    printf("%s\n\n", crow->description);


    bool printed = false;
    for ( int i = 0; i < crow->args.count; i++ ) {
        Crow_Arg* arg = crow->args.items[i];
        if ( arg->required ) {
            if ( !printed ) {
                printf("required:\n");
                printed = true;
            }
            printf("\t%-20s %s\n", arg->name, arg->desc);
        }
    }
    printed = false;
    for ( int i = 0; i < crow->args.count; i++ ) {
        Crow_Arg* arg = crow->args.items[i];
        if ( !arg->required ) {
            if ( !printed ) {
                printf("not required:\n");
                printed = true;
            }
            printf("\t%-20s %s\n", arg->name, arg->desc);
        }
    }
}

int crow_parse(Crow_Parser* crow, int argc, char* argv[])
{
    argc--;
    argv++;
    if ( argc < crow->args.count ) {
        for ( int i = argc; i < crow->args.count; i++ ) {
            Crow_Arg* arg = crow->args.items[i];
            if ( !arg->required ) { continue; }
            CROW_ERROR("not enough arguments! argument '%s' at index %d is "
                       "required.",
                arg->name, i);
            return 1;
        }
    }

    printf("%d vs %d\n", argc, crow->args.count);

    if ( 1 <= argc && !strcmp("-h", argv[0]) ) {
        crow__print_help(crow);
        return 1;
    }

    if ( crow->args.count < argc ) {
        CROW_ERROR("too many arguments provided!");
        return 1;
    }

    for ( int i = 0; i < argc; i++ ) {
        if ( !strcmp("-h", argv[i]) ) {
            crow__print_help(crow);
            return 1;
        }

        Crow_Arg* arg = crow->args.items[i];
        char* endptr; // To detect conversion errors

        switch ( arg->arg_t ) {
            case CROW_CHAR: {
                if ( argv[i][1] != '\0' ) {
                    CROW_ERROR("invalid character argument '%s'", argv[i]);
                    return 1;
                }
                arg->val.raw = argv[i][0];
                break;
            }
            case CROW_SHORT: {
                long val = strtol(argv[i], &endptr, 10);
                if ( *endptr != '\0' || val < SHRT_MIN || val > SHRT_MAX ) {
                    CROW_ERROR("invalid short integer argument '%s'", argv[i]);
                    return 1;
                }
                arg->val.raw = (short)val;
                break;
            }
            case CROW_INT: {
                long val = strtol(argv[i], &endptr, 10);

                if ( *endptr != '\0' || val < INT_MIN || val > INT_MAX ) {
                    CROW_ERROR("invalid integer argument '%s'", argv[i]);
                    return 1;
                }
                arg->val.raw = (int)val;
                break;
            }
            case CROW_LONG: {
                long val = strtol(argv[i], &endptr, 10);
                if ( *endptr != '\0' ) {
                    CROW_ERROR("invalid long integer argument '%s'", argv[i]);
                    return 1;
                }
                arg->val.raw = val;
                break;
            }
            case CROW_LLONG: {
                long long val = strtoll(argv[i], &endptr, 10);
                if ( *endptr != '\0' ) {
                    CROW_ERROR(
                        "invalid long long integer argument '%s'", argv[i]);
                    return 1;
                }
                arg->val.raw = val;
                break;
            }
            case CROW_FLOAT: {
                float val = strtof(argv[i], &endptr);
                if ( *endptr != '\0' ) {
                    CROW_ERROR("invalid float argument '%s'", argv[i]);
                    return 1;
                }
                arg->val.raw = *(uintptr_t*)&val;
                break;
            }
            case CROW_DOUBLE: {
                double val = strtod(argv[i], &endptr);
                if ( *endptr != '\0' ) {
                    CROW_ERROR("invalid double argument '%s'", argv[i]);
                    return 1;
                }
                arg->val.raw = *(uintptr_t*)&val;
                break;
            }
            case CROW_STRING: arg->val.ptr = argv[i]; break;
            default:
                CROW_ERROR("Unknown argument type for '%s'", arg->name);
                return 1;
        }
    }
    return 0;
}

void crow_close(Crow_Parser* crow)
{
    for ( int i = 0; i < crow->args.count; i++ ) {
        crow_free(crow->args.items[i]);
    }
    CROW__DAfree(crow->args);
}

void crow_get_by_name(Crow_Parser* crow, const char* name, void** out)
{
    for ( int i = 0; i < crow->args.count; i++ ) {
        Crow_Arg* arg = crow->args.items[i];
        if ( strcmp(arg->name, name) == 0 ) {
            *out = (arg->arg_t == CROW_STRING) ? arg->val.ptr : &arg->val.raw;
            return;
        }
    }
    CROW_ERROR("get by name failed, no arg with '%s' found", name);
    *out = NULL;
}

static void crow_get_indx(Crow_Parser* crow, int indx, void** out)
{
    if ( !crow || !out || indx < 0 || indx >= crow->args.count ) {
        *out = NULL;
        return;
    }

    Crow_Arg* arg = crow->args.items[indx];
    *out = (arg->arg_t == CROW_STRING) ? arg->val.ptr : &arg->val.raw;
}

void crow_add_arg(Crow_Parser* crow, const char* name, crow_arg_t arg_t,
    bool required, uintptr_t def_val, const char* desc)
{
    Crow_Arg* arg = (Crow_Arg*)crow_ptr_assert(malloc(sizeof(Crow_Arg)));

    arg->arg_t = arg_t;
    arg->required = required;
    arg->def_val = def_val;
    arg->name = name;
    arg->desc = desc;

    CROW__DAappend(crow->args, arg);
}

#        endif // !CROW_POSITIONAL
#        ifdef CROW_PARAMETRIC

struct Crow_Parser_s {
    const char* program;
    const char* description;
    Crow_Arg* args;
    // Crow_Rule* rules;
};

struct Crow_Arg_s {
    char s_flag;
    const char* l_flag;
    crow_arg_t arg_t;
    union {
        bool bool_val;
        char char_val;
        short short_val;
        int int_val;
        long long_val;
        long long llong_val;
        float float_val;
        double double_val;
        char* string_val;
    } def_val;
    int group;
    const char* desc;
};
#        endif               // !CROW_PARAMETRIC
#    endif                   // !CROW_IMPLEMENTATION

#    ifdef CROW_STRIP_PREFIX //////////////////////////////////////////////////
#        undef CROW_STRIP_PREFIX
#    endif                   // !CROW_STRIP_PREFIX

// cleanup
#    undef CROW_POSITIONAL
#    undef CROW_PARAMETRIC

#    undef CROW__DA_INIT_CAP
#    undef CROW__DA
#    undef CROW__DAinit
#    undef CROW__DAreset
#    undef CROW__DAfree
#    undef CROW__DAappend

#    undef crow_ptr_assert
#    undef crow_free
#    undef crow_malloc
#    undef crow_realloc

#    ifdef __cplusplus
}
#    endif
#endif // !_CROW_H
/*
This file is part of ape_tools.

ape_tools is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

ape_tools is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ape_tools. If not, see <https://www.gnu.org/licenses/>.
*/
