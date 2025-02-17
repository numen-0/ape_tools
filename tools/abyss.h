/*
# abyss.h - v0.0.1 - collection of memory allocators - by numen-0

 This is a single-header-file library providing utilities for memory allocation.

 For more tools, see:
 > https://github.com/numen-0/ape_tools

 To use this library, aainclude it in c or c++ file:

    // do this def each traslation unit, that requires the lib
    #define ABYSS_IMPLEMENTATION
    #include "abyss.h"


## DOCUMENTATION

### Flags
- `ABYSS_STRIP_PREFIX`

    If defined, removes the `ABYSS` prefix from API functions/macros.

- `ABYSS_UNSAFE_MODE`

    Disables safety checks and warnings.


### Allocators
 All allocators in the library share the same interface. This may be extended in
 the future with `calloc` or `realloc`, or individual allocators may have
 specific functions.

```
Abyss_ALLOCATOR* abyss_ALLOCATOR(void* buf, size_t size);
void* abyss_ALLOCATOR_alloc(Abyss_ALLOCATOR* allocator, size_t size);
void abyss_ALLOCATOR_free(Abyss_ALLOCATOR* allocator, void* ptr);
void abyss_ALLOCATOR_reset(Abyss_ALLOCATOR* allocator);
```

#### Arena Allocator
 Simple bump allocator.


#### Surge Allocator
 An extension of the Arena Allocator that keeps track, of the number of
 allocations made. As you do `abyss_surge_free`, it will decrement it's internal
 counter. If the counter reaches 0, the allocator will automatically reset the
 struct, so there's no need to manually call `abyss_surge_reset`.


### NOTES:
 * Both the Arena and Surge allocators, when asked to allocate a 0-size block,
   will return the position where the next ptr will be allocated.

   For the Surge allocator, this will not increment the internal counter. Don't
   free a 0-size allocation, as it will unsync the internal counter. `._.`


## LICENSE

   This software is licensed under the GNU General Public License v3.0 or later.
   See the end of the file for detailed license information.

## CREDITS:
 * inspired by stb-style single-header libraries.
*/

#ifndef _ABYSS_H ///////////////////////////////////////////////////////////
#    define _ABYSS_H
#    ifdef __cplusplus
extern "C" {
#    endif

#    include <stddef.h>
#    include <stdio.h>

#    define _ABYSS_WARN(msg, ...)                                          \
        fprintf(stderr, "%s:%d:abyss:warn: " msg "\n", __FILE__, __LINE__, \
            ##__VA_ARGS__)

#    ifdef ABYSS_UNSAFE_MODE
#        define _ABYSS_SAFETY_CHECK(code)
#    else
#        define _ABYSS_SAFETY_CHECK(code) \
            do {                           \
                code                       \
            } while ( 0 )
#    endif // !_ABYSS_SAFETY_CHECK

/* arena allocator ***********************************************************/

typedef struct Abyss_Arena_s Abyss_Arena;

inline static Abyss_Arena* abyss_arena(void* buf, size_t size);
inline static void* abyss_arena_alloc(Abyss_Arena* aa, size_t size);
inline static void abyss_arena_free(Abyss_Arena* aa, void* ptr);
inline static void abyss_arena_reset(Abyss_Arena* aa);

/* surge allocator ***********************************************************/

typedef struct Abyss_Surge_s Abyss_Surge;

inline static Abyss_Surge* abyss_surge(void* buf, size_t size);
inline static void* abyss_surge_alloc(Abyss_Surge* aa, size_t size);
inline static void abyss_surge_free(Abyss_Surge* aa, void* ptr);
inline static void abyss_surge_reset(Abyss_Surge* aa);

/*****************************************************************************/
#    ifdef ABYSS_IMPLEMENTATION //////////////////////////////////////////////
#        undef ABYSS_IMPLEMENTATION

/* arena allocator ***********************************************************/

// [AArena|*** allocated ***|     free     ]
//                          |              |
// AA.buf[AA.off]----------/              /
// AA.buf[AA.cap]------------------------/
struct Abyss_Arena_s {
    size_t size;
    size_t offset;
    char buf[];
};

Abyss_Arena* abyss_arena(void* buf, size_t size)
{
    _ABYSS_SAFETY_CHECK({
        if ( size < sizeof(Abyss_Arena) ) {
            _ABYSS_WARN("not enough mem provided for the arena struct...");
            return NULL;
        }
    });

    Abyss_Arena* aa = (Abyss_Arena*)buf;

    aa->size = size - sizeof(Abyss_Arena);
    aa->offset = 0;

    return aa;
}
void* abyss_arena_alloc(Abyss_Arena* aa, size_t size)
{
    if ( size == 0 ) { return &aa->buf[aa->offset]; }
    if ( aa->size - aa->offset < size ) { return NULL; }

    void* ptr = &(aa->buf[aa->offset]);
    aa->offset = aa->offset + size;

    return ptr;
}
void abyss_arena_free(Abyss_Arena* aa, void* ptr)
{
    _ABYSS_SAFETY_CHECK({
        if ( ptr == NULL ) { return; }
        if ( ptr < (void*)aa->buf || (void*)(aa->buf + aa->size) <= ptr ) {
            _ABYSS_WARN("invalid free");
            return;
        }
    });
}
void abyss_arena_reset(Abyss_Arena* aa) { aa->offset = 0; }

/* surge allocator ***********************************************************/

// [ASurge|*** allocated ***|     free     ]
//                          |              |
// AS.buf[AS.off]----------/              /
// AS.buf[AS.cap]------------------------/
// if AS.count reaches 0 it will automatically reset the allocator
struct Abyss_Surge_s {
    size_t size;
    size_t offset;
    size_t count;
    char buf[];
};

Abyss_Surge* abyss_surge(void* buf, size_t size)
{
    _ABYSS_SAFETY_CHECK({
        if ( size < sizeof(Abyss_Surge) ) {
            _ABYSS_WARN("not enough mem provided for the surge struct...");
            return NULL;
        }
    });

    Abyss_Surge* as = (Abyss_Surge*)(void*)buf;

    as->size = size;
    as->offset = 0;
    as->count = 0;

    return as;
}
void* abyss_surge_alloc(Abyss_Surge* as, size_t size)
{
    if ( size == 0 ) { return &as->buf[as->offset]; }
    if ( as->size - as->offset < size ) { return NULL; }

    void* ptr = &as->buf[as->offset];
    as->offset = as->offset + size;
    as->count++;

    return ptr;
}
void abyss_surge_free(Abyss_Surge* as, void* ptr)
{
    if ( ptr == NULL ) { return; }

    _ABYSS_SAFETY_CHECK({
        if ( ptr < (void*)as->buf
            || (void*)ptr >= (void*)(as->buf + as->size) ) {
            // Warn: invalid free
            return;
        }

        if ( as->count == 0 ) {
            // Warn: double free
            return;
        }
    });

    if ( as->count == 1 ) { as->offset = 0; }
    as->count--;
}
void abyss_surge_reset(Abyss_Surge* as)
{
    as->offset = 0;
    as->count = 0;
}

#    endif                    // !ABYSS_IMPLEMENTATION


#    ifdef ABYSS_STRIP_PREFIX ////////////////////////////////////////////
#        undef ABYSS_STRIP_PREFIX
#    endif                    // !ABYSS_STRIP_PREFIX

#    ifdef __cplusplus
}
#    endif
#endif // !_ABYSS_H

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
