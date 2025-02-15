/*
# flip.h - v0.0.1 - pointer safety & shared pointer utils - by numen-0

 This is a single-header-file library providing utilities for unique and
 shared pointers in C.

 For more tools, see:
 > https://github.com/numen-0/ape_tools

 To use this library, include it in c or c++ file:

    #include "flip.h"

 It's just a macro library (at least for now), so you don't need to ask for
 implementation in the logical units (if it where to be some definitions, I will
 do my best to make them `static`).

    // no need for this
    #define FLIP_IMPLEMENTATION
    #include "flip.h"


## DOCUMENTATION

 A simple macro library designed to help users express pointer semantics more
 clearly. As all things in C, it's just syntactic sugar, this is a special
 flavour of semantic sugar. Recommended: Read the implementation for a deeper
 understanding.

### Flags
- FLIP_NULL_AFTER_FREE

        If defined, `flip_X_free(uptr)` automatically set the pointer to `NULL`
        after freeing.

- FLIP_STRIP_PREFIX

        If defined, removes the `flip_` prefix from all api function-like
macros.


### Unique Pointers
 The `flip_unique` macro obfuscates a pointer by flipping its bits, preventing
 accidental dereferencing before explicit conversion (the conversion is
 simetrical).

```
{
    // convert a raw pointer into a unique pointer:
    int* uptr = flip_unique(malloc(sizeof(int) * 20));
    printf("rptr(%p) vs. uptr(%p)\n", flip_unique(uptr), uptr);

    { // NOTE: undefined behavior: uptr now points to an unknown address
        uptr[0] = 23;
    }

    { // to get the pointer back simply do:
        const int raw_ptr = *flip_unique_cast(int*, uptr); // recommended way
        // or:
        const int raw_ptr_ = *(int*)flip_unique(uptr);
        // to use it as an lvalue, do the same:
        flip_unique_cast(int*, uptr)[0] = 42;
    }

    { // to check if it's NULL simply do:
        if ( uptr == FLIP_UNIQUE_NULL ) { } // recommended way
        // or:
        if ( flip_unique(uptr) == NULL ) { }
        // or:
        if ( uptr == flip_unique(NULL) ) { }
    }

    { // to pass around the pointer while maintaining ownership rules:
        int* new_unpr;
        flip_unique_move(uptr, new_unpr); // uptr will be set to `NULL`
        { // NOTE: undefined behavior: uptr now points to `NULL`
            flip_unique_cast(int*, uptr)[0] = 23;
        }

        // doing the next thing defeats the purpose of using a unique pointer:
        int* unpr_copy = flip_unique(uptr);
        // if you just want an inmutable reference:
        const int* unpr_ref = flip_unique(uptr);
    }

    { // unique_free: finally to free the memory is pointing to:
        flip_unique_free(uptr); // recommended way
        // or:
        free(flip_unique(uptr));
    }
}
```

### Shared Pointers
 Interface that adds a reference counter for a ptr. Each time the pointer is
 shared the counter will increase, and each time is freed it will decrease it
 until it reaches 0 and then free it.

```
{
    { // to declare a shared pointer:
        flip_Shared_Ptr(int*) sptr_iarr;  // the type matters for `peek`,
        flip_Shared_Ptr(char*) sptr_carr; // but it can be casted
        // ...
    }

    {   // shared pointers can be declared on the stack, but if they go out of
        // scope, both the ref. count and the ptr will be lost, potentially
        flip_Shared_Ptr_s(char*) sptr_stack; // leading to memory issues.
    }

    // to create a shared pointer (the user owns this ptr, flip won't free it):
    flip_Shared_Ptr(char*) str = malloc(FLIP_SHARED_PTR_SIZE);

    { // to initialize the sptr with a ptr and sets the counter:
        flip_share_init(str, malloc(sizeof(char) * 256)); // count = 1;
    }

    { // to use it as a lvalue, we can cast it:
        for ( int i = 0; i < 10; i++ ) {
            flip_share_cast(char*, str)[i] = 'a' + i;
        }
        // or peek it, in this case the explicit cast can be ommited:
        // or you can peek the data (retrieve the raw pointer), no need for
explicit casting in this case:
        ((char*)flip_share_peek(str))[10] = '\0';
    }

    // to share ownership of the pointer (this increments the inner counter):
    flip_Shared_Ptr(char*) s_str = flip_share(str);

    printf("c: %d; str: '%s'\n", str->count, flip_share_cast(char*, str));
    printf("c: %d; str: '%s'\n", s_str->count, flip_share_cast(char*, s_str));

    { // to free the sptr (won't free until count reaches 0):
        flip_share_free(str);       // dec. count >> count = 1;
        flip_share_free(s_str);     // dec. count >> count = 0 >> free;
        flip_share_free(str);       // NOTE: undefined behavior: double free
    }
    { // to dump the shared pointer (directly free it and reset the sptr):
        flip_share_dump(str);       // free(sptr->ptr) and count = 0
        flip_share_free(str);       // NOTE: undefined behavior: double free
    }
}
```

### Contexts
 Simple static-size pointer reference tracking to enclose code in a context, and
 then free all the tracked pointers at once.

```
{
    // to define a new context, simply do:
    flip_context_open(loop_c, 20);

    for ( int i = 0; i < 24; i++ ) { // 20 < 24 -> vec2 will leak...
        // context can be defined inside other context:
        flip_context_open(inner_loop_c, 256); // remember to close them!!!

        // to add a ptr to a context, simply do:
        int* vec2 = flip_context_add(loop_c, malloc(sizeof(int) * 2));
        // or:
        flip_context_add(loop_c, malloc(20));

        { // context can be passed to functions:
            foo(inner_loop_c);
        }

        flip_context_close(inner_loop_c);
    }

    { // to free all ptrs inside the context just do:
        flip_context_close(loop_c); // be carefull of double freeing
    }

    // NOTE: if a context is overflown, it will stop tracking the new ptrs,
    //       and the program will start leaking mem :(
}
```

### NOTES:
 * It's easy to circumvent the obfuscation provided by this lib. It's not my
   intention to stop you from shooting yourself in the foot :).
 * No `NULL` checks are performed when handling pointers by the interfaces. In
   a future I might add a flag to change modes.


## LICENSE

    This software is licensed under the GNU General Public License v3.0 or
later. See the end of the file for detailed license information.

## CREDITS:
 * inspired by stb-style single-header libraries.
*/
#ifndef _FLIP_H ///////////////////////////////////////////////////////////////
#    define _FLIP_H
#    ifdef __cplusplus
extern "C" {
#    endif

#    ifndef __STDC_VERSION__
#        define __STDC_VERSION__ 199409L // Assume at least C90
#    endif
#    if __STDC_VERSION__ < 199901L
typedef unsigned long uintptr_t;         // Simple fallback for pre-C99
#    endif

#    ifdef FLIP_DEBUG
#        define flip_warn(msg, ...)                                           \
            fprintf(stderr, "%s:%d:flip:warn: " msg "\n", __FILE__, __LINE__, \
                ##__VA_ARGS__)
#    else
#        define flip_warn(...) ((void*)0)
#    endif


#    include <assert.h> // assert
#    include <stdint.h> // uintptr_t
#    include <stdlib.h> // free, malloc, realloc

#    ifdef FLIP_NULL_AFTER_FREE
#        undef FLIP_NULL_AFTER_FREE
#        define flip_free(ptr) \
            do {               \
                free(ptr);     \
                ptr = NULL;    \
            } while ( 0 )
#    else
#        define flip_free(ptr) free(ptr)
#    endif //! FLIP_NULL_AFTER_FREE

#    define flip_assert  assert
#    define flip_malloc  malloc
#    define flip_realloc realloc

#    define _FLIP__ARR_SIZE(arr) (sizeof(arr) / sizeof(*arr))

#    define FLIP_UNUSED(var) ((void)var)
#    define FLIP_TODO(msg)   fprintf(stderr, "flip: TODO: " msg "\n")

/* unique pointers ***********************************************************/

#    define FLIP_UNIQUE_NULL flip_unique(NULL)

#    define flip_unique(ptr)            ((void*)(~(uintptr_t)ptr))
#    define flip_unique_cast(type, ptr) ((type)(void*)(~(uintptr_t)ptr))
#    define flip_unique_peek(ptr)       flip_unique(ptr)

#    define flip_unique_free(uptr) flip_free((void*)(~(uintptr_t)uptr))
#    define flip_unique_move(uptr, ptr) \
        do {                            \
            ptr = uptr;                 \
            uptr = NULL;                \
        } while ( 0 )

/* shared pointers ***********************************************************/

#    define FLIP_SHARED_PTR_SIZE \
        sizeof(struct {          \
            uint32_t count;      \
            void* ptr;           \
        })
#    define flip_Shared_Ptr_s(type) \
        struct {                    \
            uint32_t count;         \
            type ptr;               \
        }
#    define flip_Shared_Ptr(type) \
        struct {                  \
            uint32_t count;       \
            type ptr;             \
        }*

#    define flip_share(sptr)            ((sptr)->count++, ((void*)(sptr)))
#    define flip_share_peek(sptr)       ((sptr)->ptr)
#    define flip_share_cast(type, sptr) ((type)(sptr)->ptr)

#    define flip_share_init(sptr, p)                  \
        ((void*)((sptr)->count = 1, (sptr)->ptr = p))
#    define flip_share_dump(sptr)                                           \
        do {                                                                \
            flip_assert((sptr)->count > 0 && "flip: double free detected"); \
            flip_free((sptr)->ptr);                                         \
            (sptr)->count = 0;                                              \
        } while ( 0 )
#    define flip_share_free(sptr)                                           \
        do {                                                                \
            flip_assert((sptr)->count > 0 && "flip: double free detected"); \
            if ( --((sptr)->count) == 0 ) { flip_free((sptr)->ptr); }       \
        } while ( 0 )

/* contexts ******************************************************************/

#    define flip_context_open(cname, size) \
        struct {                           \
            size_t count;                  \
            void* entries[size];           \
        } cname = {                        \
            .count = 0,                    \
        }
#    define flip_context_close(cname)                                  \
        do {                                                           \
            for ( size_t _indx = 0; _indx < (cname).count; _indx++ ) { \
                flip_free((cname).entries[_indx]);                     \
            }                                                          \
        } while ( 0 )
#    define flip_context_add(cname, ptr)                                      \
        ((cname).count < _FLIP__ARR_SIZE((cname).entries)                     \
                ? ((cname).entries[(cname).count++] = ptr)                    \
                : (flip_warn(                                                 \
                       "context '" #cname "' too small, leaking memory ._."), \
                      ptr))

/* block *********************************************************************/
#    if 0 // NOTE: this is highly experimental
#        define flip_block_open(bname, size)                  \
            struct {                                          \
                size_t count;                                 \
                size_t cap;                                   \
                void** entries;                               \
            } bname = {                                       \
                .count = 0,                                   \
                .cap = size,                                  \
                .entries = flip_malloc(sizeof(void*) * size), \
            }

#        define flip_block_close(bname)                                    \
            do {                                                           \
                if ( (bname).entries == NULL ) { break; }                  \
                for ( size_t _indx = 0; _indx < (bname).count; _indx++ ) { \
                    if ( (bname).entries[_indx] == NULL ) { continue; }    \
                    flip_free((bname).entries[_indx]);                     \
                }                                                          \
                flip_free((bname).entries);                                \
            } while ( 0 )

// NOTE: if we do flip_block_add(my_b, malloc(n)), malloc will be called once
//       it's really ugly, but is this or make a function and I choose
violence
#        define flip_block_add(bname, ptr)                                     \
            ((bname).entries == NULL)                                          \
                ? (flip_warn("block '" #bname                                  \
                             "' failed to realloc, leaking memory ._."),       \
                      ptr)                                                     \
                : ((bname).count < (bname).cap)                                \
                ? ((bname).entries[(bname).count++] = ptr)                     \
                : (((bname).entries                                            \
                       = (void**)realloc((bname).entries, ((bname).cap *= 2))) \
                      != NULL)                                                 \
                ? ((bname).entries[(bname).count++] = ptr)                     \
                : (flip_warn("block '" #bname                                  \
                             "' failed to realloc, leaking memory ._."),       \
                      (bname).entries = NULL, ptr)
#    endif

/* weak pointers *************************************************************/
// TODO:

/*****************************************************************************/

#    ifdef FLIP_STRIP_PREFIX //////////////////////////////////////////////////
#        undef FLIP_STRIP_PREFIX
#        define share       flip_share
#        define shared_free flip_shared_free
#        define shared_ptr  flip_shared_ptr
#        define unique      flip_unique
#        define UNIQUE_NULL FLIP_UNIQUE_NULL
#        define unique_free flip_unique_free
#    endif // !FLIP_STRIP_PREFIX

#    ifdef __cplusplus
}
#    endif
#endif // !_FLIP_H

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
