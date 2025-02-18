/*
# abyss.h - v0.3 - collection of memory allocators - by numen-0

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

- `ABYSS_DATA_ALIGN`

    Defines the alignment in bytes for the allocated blocks (default: 8 bytes).

- `ABYSS_THREAD_SAFE_MODE`

    If enabled, ensures that allocators work safely in multithreaded
    environments. This activates mutex locking for memory operations, protecting
    shared memory from race conditions.

### Allocators
 Allocators **do not** use `malloc` or `mmap`. Instead the user provides a
 memory block and it's size, and the allocator manages that memory within the
 given block. The allocator resides within the same memory, so there is no need
 to explicitly destroy or free it.

 However, if the provided memory was heap-allocated, the user is responsible for
 freeing or unmaping it when the allocator is no longer needed.

 All allocators in the library share the same interface. This may be extended in
 the future with `calloc` or `realloc`, or individual allocators may have
 specific functions.

```
Abyss_ALLOCATOR* abyss_ALLOCATOR_init(void* buf, size_t size);
void* abyss_ALLOCATOR_alloc(Abyss_ALLOCATOR* allocator, size_t size);
void abyss_ALLOCATOR_free(Abyss_ALLOCATOR* allocator, void* ptr);
void abyss_ALLOCATOR_reset(Abyss_ALLOCATOR* allocator);

// NOTE: if `ABYSS_THREAD_SAFE_MODE` is activate, this function will be abaible,
//       and it must be used when you are done with the allocator. It destrois
//       the mutexes the allocators use.
void abyss_ALLOCATOR_destroy(Abyss_ALLOCATOR* allocator);

{ // example with the Arena allocator:
    // init the allocator:
    Abyss_Arena* aa = abyss_arena_init(malloc(256), 256);

    { // or we can do this trict to have memory from the stack:
        // NOTE: if _buf goes out of scope, using the allocator it will be UB.
        char _buf[256];
        Abyss_Arena* aa = abyss_arena_init(_buf, 256);
    }

    { // allocate and free a block from the arena:
        // NOTE: it ca fail if there is not enough mem in the allocator,
        //       insttead off returning a valid pointer it will return `NULL`.
        char* str = abyss_arena_alloc(aa, sizeof(char) * 16);

        // when we are done with the ptr allocated from the allocator, we do.
        abyss_arena_free(aa, str);
    }

    { // if you are done using the allocator:
        // NOTE: now is the user responsibility to free the given memory block.
        //       All alocators are placed at the bottom of the block, for
        //       example if you did:
        Abyss_Arena* aa = abyss_arena_init(malloc(256), 256);

        // you simply do:
        free(aa);

        // NOTE: if gave stack memory block, simply don't do nothing.
    }

    { // if you want to reset the allocator for further use down the program:
        abyss_arena_reset(aa);

        // NOTE: using any of the previouslly allocated memory block in aa, will
        //       be UB.
    }
}
```

#### Arena Allocator
 Simple bump allocator.

```
{ // HACK: if you want to change the allocator size, if this is **empty**,
    //      after a reset or init, you can just do (be carefull realloc can
fail) aa = abyss_arena_init(realloc(aa, new_size), new_size);
}
```


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
 * Inspired by stb-style single-header libraries.
*/

#ifndef _ABYSS_H //////////////////////////////////////////////////////////////
#    define _ABYSS_H
#    ifdef __cplusplus
extern "C" {
#    endif

#    include <stddef.h>

/* arena allocator ***********************************************************/

typedef struct Abyss_Arena_s Abyss_Arena;

inline static Abyss_Arena* abyss_arena_init(void* buf, size_t size);
inline static void* abyss_arena_alloc(Abyss_Arena* aa, size_t size);
inline static void abyss_arena_free(Abyss_Arena* aa, void* ptr);
inline static void abyss_arena_reset(Abyss_Arena* aa);

/* surge allocator ***********************************************************/

typedef struct Abyss_Surge_s Abyss_Surge;

inline static Abyss_Surge* abyss_surge_init(void* buf, size_t size);
inline static void* abyss_surge_alloc(Abyss_Surge* aa, size_t size);
inline static void abyss_surge_free(Abyss_Surge* aa, void* ptr);
inline static void abyss_surge_reset(Abyss_Surge* aa);

/* allocator balancer ********************************************************/

typedef struct Abyss_Surge_s Abyss_Surge;

inline static Abyss_Surge* abyss_surge_init(void* buf, size_t size);
inline static void* abyss_surge_alloc(Abyss_Surge* aa, size_t size);
inline static void abyss_surge_free(Abyss_Surge* aa, void* ptr);
inline static void abyss_surge_reset(Abyss_Surge* aa);

/*****************************************************************************/

#    ifdef ABYSS_THREAD_SAFE_MODE
inline static void abyss_arena_destroy(Abyss_Arena* aa);
inline static void abyss_surge_destroy(Abyss_Surge* as);
#    endif // !ABYSS_THREAD_SAFE_MODE

/*****************************************************************************/

#    ifdef ABYSS_IMPLEMENTATION ///////////////////////////////////////////////
#        undef ABYSS_IMPLEMENTATION

#        ifdef ABYSS_UNSAFE_MODE
#            define _ABYSS_SAFETY_CHECK(code)
#        else
#            include <stdio.h>
#            define _ABYSS_SAFETY_CHECK(code) \
                do {                          \
                    code                      \
                } while ( 0 )
#        endif // !_ABYSS_SAFETY_CHECK

#        ifdef ABYSS_THREAD_SAFE_MODE
#            include <pthread.h>
#            include <stdio.h>
#            define _ABYSS_THREAD_SAFE(code) \
                do {                         \
                    code                     \
                } while ( 0 )
#            define _ABYSS_ADD_LOCK(lock) pthread_mutex_t lock
#        else
#            define _ABYSS_THREAD_SAFE(code)
#            define _ABYSS_ADD_LOCK(lock)
#        endif // !ABYSS_THREAD_SAFE_MODE

#        define _ABYSS_WARN2(msg, ...)                                         \
            fprintf(stderr, "%s:%d:abyss:warn: " msg "\n", __FILE__, __LINE__, \
                ##__VA_ARGS__)
#        define _ABYSS_WARN(msg) _ABYSS_WARN2(msg "%s", "")

#        ifndef ABYSS_DATA_ALIGN
#            define ABYSS_DATA_ALIGN 8
#        endif // !ABYSS_DATA_ALIGN
#        define _ABYSS_ROUND_UP(n, align) (((n) + (align) - 1) & -(align))

/* arena allocator ***********************************************************/

// [AArena |*** allocated ***|     free     ]
// AA.buf-/                  |              |
// AA.buf[AA.off]-----------/              /
// AA.buf[AA.cap]-------------------------/
struct Abyss_Arena_s {
    size_t size;
    size_t offset;
    _ABYSS_ADD_LOCK(lock);
    char buf[];
};

Abyss_Arena* abyss_arena_init(void* buf, size_t size)
{
    _ABYSS_SAFETY_CHECK({
        if ( size < sizeof(Abyss_Arena) ) {
            _ABYSS_WARN("not enough mem provided for the arena struct...");
            return NULL;
        }
    });

    Abyss_Arena* aa = (Abyss_Arena*)buf;

    _ABYSS_THREAD_SAFE({
        if ( pthread_mutex_init(&aa->lock, NULL) != 0 ) { // init mutex
            _ABYSS_WARN("mutex initialization failed");
            return NULL;
        }
    });

    aa->size = size - sizeof(Abyss_Arena);
    // align the buf with padding, by adding the offset
    aa->offset = _ABYSS_ROUND_UP(sizeof(Abyss_Arena), ABYSS_DATA_ALIGN)
        - sizeof(Abyss_Arena);

    return aa;
}
void* abyss_arena_alloc(Abyss_Arena* aa, size_t size)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&aa->lock); });

    if ( size == 0 ) { return &aa->buf[aa->offset]; }
    if ( aa->size - aa->offset < size ) { return NULL; }

    void* ptr = &(aa->buf[aa->offset]);
    aa->offset = _ABYSS_ROUND_UP(aa->offset + size, ABYSS_DATA_ALIGN);

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&aa->lock); });

    return ptr;
}
void abyss_arena_free(Abyss_Arena* aa, void* ptr)
{
    _ABYSS_SAFETY_CHECK({
        if ( ptr == NULL ) { return; }

        _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&aa->lock); });

        if ( ptr < (void*)aa->buf || (void*)(aa->buf + aa->offset) <= ptr ) {
            _ABYSS_WARN(
                "invalid free, out of the allocated block mem range, it could "
                "be a double free, or a pointer that wasn't allocated in this "
                "allocator.");
        }

        _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&aa->lock); });
    });
}
void abyss_arena_reset(Abyss_Arena* aa)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&aa->lock); });

    aa->offset = _ABYSS_ROUND_UP(sizeof(Abyss_Arena), ABYSS_DATA_ALIGN)
        - sizeof(Abyss_Arena);

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&aa->lock); });
}

/* surge allocator ***********************************************************/

// [ASurge |*** allocated ***|     free     ]
// AS.buf-/                  |              |
// AS.buf[AS.off]-----------/              /
// AS.buf[AS.cap]-------------------------/
// if AS.count reaches 0 it will automatically reset the allocator
struct Abyss_Surge_s {
    size_t size;
    size_t offset;
    size_t count;
    _ABYSS_ADD_LOCK(lock);
    char buf[];
};

Abyss_Surge* abyss_surge_init(void* buf, size_t size)
{
    _ABYSS_SAFETY_CHECK({
        if ( size < sizeof(Abyss_Surge) ) {
            _ABYSS_WARN("not enough mem provided for the surge struct...");
            return NULL;
        }
    });

    Abyss_Surge* as = (Abyss_Surge*)buf;

    _ABYSS_THREAD_SAFE({
        if ( pthread_mutex_init(&as->lock, NULL) != 0 ) {
            _ABYSS_WARN("mutex initialization failed");
            return NULL;
        }
    });

    as->size = size - sizeof(Abyss_Surge);
    // align the buf with padding, by adding the offset
    as->offset = _ABYSS_ROUND_UP(sizeof(Abyss_Arena), ABYSS_DATA_ALIGN)
        - sizeof(Abyss_Arena);
    as->count = 0;

    return as;
}
void* abyss_surge_alloc(Abyss_Surge* as, size_t size)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&as->lock); });
    if ( size == 0 ) { return &as->buf[as->offset]; }
    if ( as->size - as->offset < size ) { return NULL; }

    void* ptr = &as->buf[as->offset];
    as->offset = _ABYSS_ROUND_UP(as->offset + size, ABYSS_DATA_ALIGN);
    as->count++;

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&as->lock); });
    return ptr;
}
void abyss_surge_free(Abyss_Surge* as, void* ptr)
{
    if ( ptr == NULL ) { return; }

    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&as->lock); });

    _ABYSS_SAFETY_CHECK({
        if ( ptr < (void*)as->buf
            || (void*)ptr >= (void*)(as->buf + as->offset) ) {
            _ABYSS_WARN(
                "invalid free, out of the allocated block mem range, it could "
                "be a double free, or a pointer that wasn't allocated in this "
                "allocator.");
            _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&as->lock); });
            return;
        }

        if ( as->count == 0 ) {
            _ABYSS_WARN("invalid double free");
            _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&as->lock); });
            return;
        }
    });

    if ( as->count == 1 ) {
        as->offset = _ABYSS_ROUND_UP(sizeof(Abyss_Arena), ABYSS_DATA_ALIGN)
            - sizeof(Abyss_Arena);
    }
    as->count--;

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&as->lock); });
}
void abyss_surge_reset(Abyss_Surge* as)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&as->lock); });

    as->offset = _ABYSS_ROUND_UP(sizeof(Abyss_Arena), ABYSS_DATA_ALIGN)
        - sizeof(Abyss_Arena);
    as->count = 0;

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&as->lock); });
}

/*****************************************************************************/

#        ifdef ABYSS_THREAD_SAFE_MODE

void abyss_arena_destroy(Abyss_Arena* aa)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_destroy(&aa->lock); });
}
void abyss_surge_destroy(Abyss_Surge* as)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_destroy(&as->lock); });
}

#        endif // !ABYSS_THREAD_SAFE_MODE
#    endif     // !ABYSS_IMPLEMENTATION

/*****************************************************************************/

#    ifdef ABYSS_STRIP_PREFIX /////////////////////////////////////////////////
#        undef ABYSS_STRIP_PREFIX

#        define Arena       Abyss_Arena
#        define arena_init  abyss_arena_init
#        define arena_alloc abyss_arena_alloc
#        define arena_free  abyss_arena_free
#        define arena_reset abyss_arena_reset

#        define Surge       Abyss_Surge
#        define surge_init  abyss_surge_init
#        define surge_alloc abyss_surge_alloc
#        define surge_free  abyss_surge_free
#        define surge_reset abyss_surge_reset

#        ifdef ABYSS_THREAD_SAFE_MODE
#            define arena_destroy abyss_arena_destroy
#            define surge_destroy abyss_surge_destroy
#        endif // !ABYSS_THREAD_SAFE_MODE

#    endif     // !ABYSS_STRIP_PREFIX

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
