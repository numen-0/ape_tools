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


## TABLE OF CONTENTS
- documentation
    - compile-time options
    - unique pointers
    - shared pointers
    - notes
- license
- credits

## DOCUMENTATION

### COMPILE-TIME OPTIONS
- FLIP_NULL_AFTER_FREE

        if defined, `flip_unique_free(uptr)` and `flip_shared_free(sptr)` will
        set `ptr = NULL` after freeing it.

- FLIP_STRIP_PREFIX

        if defined, removes the `flip_` prefix from all function-like macros.


### Unique Pointers

- flip_unique_ptr(type, name, ptr)

        declares a unique pointer of type `type`.

- flip_unique(type, ptr)

        converts a raw pointer to a unique pointer.

- flip_unique_free(uptr)

        frees a unique pointer.

- flip_unique_NULL

        defines a `NULL` value for unique pointers.


### Shared Pointers

- flip_shared_ptr(type, name, init_ptr)

        declares a shared pointer with reference counting.


- flip_share(sptr)

        increments the reference count and returns the pointer.

- flip_shared_free(sptr)

        decrements the reference count and frees the memory if zero.

### NOTES:
 * Unique pointers are bitwise-inverted, meaning directly using them may break
   your code. Instead, use the provided interface.
 * No NULL checks are performed when handling pointers by the interfaces.
 * It's easy to circumvent the obfuscation provided by this lib. It's not my
   intention to stop you from shooting yourself in the foot :).


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

#ifndef __STDC_VERSION__
#define __STDC_VERSION__ 199409L  // Assume at least C90
#endif
#if __STDC_VERSION__ < 199901L
typedef unsigned long uintptr_t;  // Simple fallback for pre-C99
#endif

#    include <assert.h> // assert
#    include <stdint.h> // uintptr_t
#    include <stdlib.h> // free

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

#    define flip_assert assert

/* unique pointers ***********************************************************/

#    define flip_unique(ptr)                 (void*)(~(uintptr_t)ptr)
#    define flip_unique_NULL                 flip_unique(NULL)
#    define flip_unique_ptr(type, name, ptr) type* name = flip_unique(ptr)
#    define flip_unique_free(uptr)           flip_free((void*)(~(uintptr_t)uptr))

/* shared pointers ***********************************************************/

#    define flip_share(sptr) ((sptr).count++, (sptr).p)
#    define flip_shared_ptr(type, name, init_ptr) \
        struct {                                  \
            type* p;                              \
            uint32_t count;                       \
        } name = {                                \
            .p = (init_ptr),                      \
            .count = 1,                           \
        };
#    define flip_shared_free(sptr)                                          \
        do {                                                                \
            flip_assert((sptr).count < 1 && "flip: doucle free detected"); \
            /* NOTE: this frees sptr.p not sptr */                          \
            if ( --(sptr).count == 0 ) { flip_free((sptr).p); }             \
        } while ( 0 )

// #ifdef FLIP_IMPLEMENTATION /////////////////////////////////////////////////
// #    undef FLIP_IMPLEMENTATION
// #endif // !FLIP_IMPLEMENTATION

#    ifdef FLIP_STRIP_PREFIX //////////////////////////////////////////////////////
#        undef FLIP_STRIP_PREFIX
#        define share       flip_share
#        define shared_free flip_shared_free
#        define shared_ptr  flip_shared_ptr
#        define unique      flip_unique
#        define unique_NULL flip_unique_NULL
#        define unique_free flip_unique_free
#        define unique_ptr  flip_unique_ptr
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
