/*
# abyss.h - v0.4 - collection of memory allocators - by numen-0

 This is a single-header-file library providing utilities for memory allocation.

 For more tools, see:
 > https://github.com/numen-0/ape_tools

 To use this library, aainclude it in c or c++ file:

    // do this def each traslation unit, that requires the lib
    #define ABYSS_IMPLEMENTATION
    #include "abyss.h"


## DOCUMENTATION

### Flags
- `ABYSS_DATA_ALIGN`

    Defines the alignment in bytes for the allocated blocks (default: 8 bytes).

- `ABYSS_STRIP_PREFIX`

    If enabled, removes the `ABYSS` prefix from API functions/macros.

- `ABYSS_UNSAFE_MODE`

    If enablde, disables safety checks and warnings. (not the ones for thread
safety)

- `ABYSS_THREAD_SAFE_MODE`

    If enabled, ensures that allocators work safely in multithreaded
    environments. This activates mutex locking for memory operations, protecting
    shared memory from race conditions.

- `ABYSS_HANDLE_MODE`

    If enabled, this mode changes the allocators interface. Instead of returning
    pointer, the allocators will return a offset (`size_t`). This can be
    helpfull for serializing the program. The offset is relative to the
    allocator address, meaning, at the start of buffer you passed.

    By using abyss_handle, we can flip the flag as needed. This ensures
    flexibility when switching between pointer-based and offset-based
    allocation.

    For more information, see the example below:
```
#define ABYSS_HANDLE_MODE
#define ABYSS_IMPLEMENTATION
#include "abyss.h"

{ // example usage of `abyss_handle`:
    // init the allocator:
    Abyss_Arena* arena = abyss_arena_init(malloc(256), 256);

    { // NOTE: `abyss_handle` is `void*` when `ABYSS_HANDLE_MODE` is off, and
        //     `size_t` when is on.
        abyss_handle hdl = abyss_arena_alloc(arena, sizeof(char) * 16);

        { // NOTE: The allocation may fail and return `NULL` (if pointer are
            //     used) or `0` (if offests are used). To check do this:
            if ( abyss_handle_NULL == hdl ) { }
            // or:
            if ( abyss_handle_is_NULL(hdl) ) { }
        }

        { // to get the pointer from the hdl., use:
            char* str = abyss_handle_get_ptr(arena, hdl);
            // or:
            char* str = abyss_handle_cast(char*, arena, hdl);
        }

        { // allocation and access in in a sigle line:
            int* arr = abyss_handle_get_ptr(
                arena, abyss_arena_alloc(arena, sizeof(int) * 32));
            if ( arr == NULL ) { }
        }

        { // if we need to keep a reference to another reference it could be
            // either a pointer or an offset:
            abyss_handle hook = abyss_handle_rel(hdl, 16);
        }

        { // the usage with the interfaces is the same, see below for more:
            abyss_arena_free(arena, hdl);
        }
    }
    // NOTE: Using `abyss_handle` is entirely optional. It simply provides tools
    //       to ensure compatibility when toggling `ABYSS_HANDLE_MODE` on or
    //       off without breaking the code.
}
```

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
#ifdef ABYSS_HANDLE_MODE
#    define abyss_handle size_t
#else
#    define abyss_handle void*
#endif // !ABYSS_HANDLE_MODE

Abyss_ALLOCATOR* abyss_allocator_init(void* buf, size_t size);
abyss_handle abyss_allocator_alloc(Abyss_ALLOCATOR* allocator, size_t size);
void abyss_allocator_free(Abyss_ALLOCATOR* allocator, abyss_handle hdl);
void abyss_allocator_reset(Abyss_ALLOCATOR* allocator);

// NOTE: if `ABYSS_THREAD_SAFE_MODE` is activate, this function will be abaible,
//       and it must be used when you are done with the allocator. It destrois
//       the mutexes the allocators use.
void abyss_ALLOCATOR_destroy(Abyss_ALLOCATOR* allocator);

{ // example with the Arena allocator:
    // init the allocator:
    Abyss_Arena* arena = abyss_arena_init(malloc(256), 256);

    { // or we can do this trict to have memory from the stack:
        // NOTE: if _buf goes out of scope, using the allocator it will be UB.
        char _buf[256];
        Abyss_Arena* arena = abyss_arena_init(_buf, 256);
    }

    { // allocate and free a block from the arena:
        // NOTE: it ca fail if there is not enough mem in the allocator,
        //       insttead off returning a valid pointer it will return `NULL`.
        char* str = abyss_arena_alloc(arena, sizeof(char) * 16);

        // when we are done with the ptr allocated from the allocator, we do.
        abyss_arena_free(arena, str);
    }

    { // if you are done using the allocator:
        // NOTE: now is the user responsibility to free the given memory block.
        //       All alocators are placed at the bottom of the block, for
        //       example if you did:
        Abyss_Arena* arena = abyss_arena_init(malloc(256), 256);

        // you simply do:
        free(arena);

        // NOTE: if gave stack memory block, simply don't do nothing.
    }

    { // if you want to reset the allocator for further use down the program:
        abyss_arena_reset(arena);

        // NOTE: using any of the previouslly allocated memory block in arena,
        //       will be undefined behavior (UB).
    }
}
```

#### Arena Allocator
 Simple bump allocator.

```
{ // HACK: If you want to change the allocator size it can be done under two
  //       conditions:
  //       1. The allocator is **empty** after a reset or an init without any
  //          allocations.
  //       2. `ABYSS_HANDLE_MODE` is enabled, and only handles are being used.
  //          If a handle is dereferenced (i.e., converted to a raw pointer) and
  //          stored in a variable, that pointers may become invalid, and using
  //          it is undefined behavior (UB).
  // [1]: arena is empty
  void* new_arena = realloc(arena, new_size);
  assert(new_arena != NULL); // remember realloc could fail.
  arena = abyss_arena_init(new_arena, new_size);

  // [2]: `ABYSS_HANDLE_MODE` enabled
  void* new_arena = realloc(arena, new_size);
  assert(new_arena != NULL);
  arena = new_arena;
  arena->size = new_size - sizeof(Abyss_Arena);
}
```

#### Surge Allocator
 An extension of the Arena Allocator that keeps track, of the number of
 allocations made. As you do `abyss_surge_free`, it will decrement it's internal
 counter. If the counter reaches 0, the allocator will automatically reset the
 struct, so there's no need to manually call `abyss_surge_reset`.

#### Totem Allocator
 The `Abyss_Totem` is an allocator agglomerator that stacks multiple allocators
 inside it and processes requests in Last-In, First-Out (LIFO) order. It can
 function as a "dynamic" allocator.

 NOTE: **Highly experimental**: May be changed or removed in a future update.
 NOTE: **No Circular References**: Never push a totem inside itself

```
{
    Abyss_Totem* totem;

    { // initialize the totem, giving it space for 4 allocator
        const size_t size = ABYSS_TOTEM_SIZE(4);
        totem = abyss_totem_init(malloc(size), size);
    }
    { // create first arena
        Abyss_Arena* aa = abyss_arena_init(malloc(256), 256);
        abyss_totem_push(totem, aa, ABYSS_ARENA_T);
    }

    {
        for ( int i = 0; i < 32; i++ ) {
            abyss_handle hdl = abyss_totem_alloc(totem, 8);

            // if the arenas inise are full, just push and make a new one.
            if ( abyss_handle_NULL == hdl ) {
                Abyss_Arena* aa = abyss_arena_init(malloc(256), 256);
                void* t = abyss_totem_push(totem, aa, ABYSS_ARENA_T);
                if ( t == NULL ) { // Remember the 4 allocator limit
                    const size_t size = ABYSS_TOTEM_SIZE(4);
                    Abyss_Totem* t2 = abyss_totem_init(malloc(size), size);
                    // we push the old totem, and the new arena
                    abyss_totem_push(t2, totem, ABYSS_TOTEM_T);
                    abyss_totem_push(t2, aa, ABYSS_ARENA_T);
                }
                // finally we get the new ptr
                hdl = abyss_totem_alloc(totem, 8);
            }

            // If a pointer needs to be freed, the totem will search for the
            // allocator that contains it and call `free` on that allocator.
            //
            // This feature conflicts with the library's handle-based
            // approach. `abyss_totem_free` only accepts raw pointers, even
            // in `ABYSS_HANDLE_MODE`. However, converting a handle back
            // into a pointer requires the original allocator, which makes
            // this process quite messy. While the totem might work as a
            // pump-and-dump allocator, freeing individual pointers becomes
            // a headache...
            void* ptr = abyss_handle_get_ptr(??, hdl);
            abyss_totem_free(totem, ptr);
        }
    }
    { // NOTE: this call will reset all the allocators inside itself
        abyss_totem_reset(totem);
    }
    { // If an allocator inside the totem needs to be freed:
        // If the index is negative; `index' = count + index`
        void* allocator = abyss_totem_pop(totem, -1, NULL);
        free(allocator);
    }
    { // finally to destroy & free the totem (and it's allocator)
        abyss_totem_destroy(totem);

        void free_all(void* totem) // Example funcion
        {
            void* allocator;
            int type;
            while ( (allocator = abyss_totem_pop(totem, 0, &type)) != NULL ) {
                if ( type == ABYSS_TOTEM_T ) {
                    free_all(allocator);
                } else {
                    free(allocator);
                }
            }
            free(totem);
        }
        free_all(totem); // Remember to free the allocators inside
    }
}
```

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

#    ifdef ABYSS_HANDLE_MODE
#        undef ABYSS_HANDLE_MODE
#        define abyss_handle size_t
#        define abyss_handle_get_hdl(allocator, ptr)                      \
            ((ptr == NULL) ? 0 : (size_t)((char*)ptr - (char*)allocator))
#        define abyss_handle_get_ptr(allocator, hdl)                \
            ((hdl == 0) ? NULL : (void*)(((char*)allocator) + hdl))
#        define abyss_handle_cast(type, allocator, hdl)    \
            ((type)(abyss_handle_get_ptr(allocator, hdl)))
#        define abyss_handle_rel(hdl, off)    ((hdl) ? ((hdl) + (off)) : 0)
#        define abyss_handle_switch(ptr, off) (off)
#        define abyss_handle_NULL             (0)
#        define abyss_handle_is_NULL(val)     (val == 0)
#    else
#        define abyss_handle                            void*
#        define abyss_handle_get_hdl(allocator, ptr)    (ptr)
#        define abyss_handle_get_ptr(allocator, hdl)    (hdl)
#        define abyss_handle_cast(type, allocator, hdl) ((type)(hdl))
#        define abyss_handle_rel(hdl, off)                     \
            ((hdl) ? ((void*)(((char*)(hdl)) + (off))) : NULL)
#        define abyss_handle_switch(ptr, hdl) (ptr)
#        define abyss_handle_NULL             (NULL)
#        define abyss_handle_is_NULL(val)     (val == NULL)
#    endif // !ABYSS_HANDLE_MODE

#    ifndef ADEF
#        define ADEF inline static
#    endif // !ADEF

/*****************************************************************************/

typedef enum {
    ABYSS_SURGE_T = 1,
    ABYSS_ARENA_T,
    ABYSS_TOTEM_T,
} abyss_allocator_t;

/* arena allocator ***********************************************************/

typedef struct Abyss_Arena_s Abyss_Arena;

ADEF Abyss_Arena* abyss_arena_init(void* buf, size_t size);
ADEF abyss_handle abyss_arena_alloc(Abyss_Arena* arena, size_t size);
ADEF void abyss_arena_free(Abyss_Arena* arena, abyss_handle hdl);
ADEF void abyss_arena_reset(Abyss_Arena* arena);

/* surge allocator ***********************************************************/

typedef struct Abyss_Surge_s Abyss_Surge;

ADEF Abyss_Surge* abyss_surge_init(void* buf, size_t size);
ADEF abyss_handle abyss_surge_alloc(Abyss_Surge* surge, size_t size);
ADEF void abyss_surge_free(Abyss_Surge* surge, abyss_handle hdl);
ADEF void abyss_surge_reset(Abyss_Surge* surge);

/* allocator totem ***********************************************************/

// NOTE: there is no need for round up
#    define ABYSS_TOTEM_SIZE(capacity)                   \
        (sizeof(Abyss_Totem) + sizeof(void*) * capacity)

typedef struct Abyss_Totem_s Abyss_Totem;

ADEF Abyss_Totem* abyss_totem_init(void* buf, size_t size);
ADEF abyss_handle abyss_totem_alloc(Abyss_Totem* totem, size_t size);
ADEF void abyss_totem_free(Abyss_Totem* totem, void* hdl);
ADEF void abyss_totem_reset(Abyss_Totem* totem);

ADEF void* abyss_totem_push(
    Abyss_Totem* totem, void* allocator, abyss_allocator_t type);
ADEF void* abyss_totem_pop(Abyss_Totem* totem, int indx, int* type);

/* allocator balancer ********************************************************/
// NOTE: if I do this one, it will me a abstraction mess, look at totem...
//       balancers may be cool but don't make them generic. It will force you
//       to extend the lib basic interface for all the allocators...

/*****************************************************************************/

#    ifdef ABYSS_THREAD_SAFE_MODE
ADEF void abyss_arena_destroy(Abyss_Arena* arena);
ADEF void abyss_surge_destroy(Abyss_Surge* surge);
ADEF void abyss_totem_destroy(Abyss_Totem* totem);
#    endif // !ABYSS_THREAD_SAFE_MODE

/*****************************************************************************/

#    ifdef ABYSS_IMPLEMENTATION ///////////////////////////////////////////////
#        undef ABYSS_IMPLEMENTATION

#        ifndef __STDC_VERSION__
#            define __STDC_VERSION__ 199409L // Assume at least C90
#        endif
#        if __STDC_VERSION__ < 199901L
typedef long intptr_t;                       // Simple fallback for pre-C99
#        endif

#        include <stdint.h>                  // intptr_t

#        ifdef ABYSS_UNSAFE_MODE
#            undef ABYSS_UNSAFE_MODE
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
#        define _ABYSS_ROUND_S(allocator)                        \
            _ABYSS_ROUND_UP(sizeof(allocator), ABYSS_DATA_ALIGN)

/* private helper funcions ***************************************************/

ADEF int _abyss_arena_contains(Abyss_Arena* arena, void* ptr);
ADEF int _abyss_surge_contains(Abyss_Surge* surge, void* ptr);
ADEF int _abyss_totem_contains(Abyss_Totem* totem, void* ptr);

/* arena allocator ***********************************************************/

// [AArena |*** allocated ***|     free     ]
// AA.buf-/                 /              /
// AA.buf[AA.off]----------/              /
// AA.buf[AA.size]-----------------------/
struct Abyss_Arena_s {
    size_t offset;
    size_t size;
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

    Abyss_Arena* arena = (Abyss_Arena*)buf;

    _ABYSS_THREAD_SAFE({
        if ( pthread_mutex_init(&arena->lock, NULL) != 0 ) { // init mutex
            _ABYSS_WARN("mutex initialization failed");
            return NULL;
        }
    });

    arena->size = size - sizeof(Abyss_Arena);
    // align the buf with padding, by adding the offset
    arena->offset = _ABYSS_ROUND_S(Abyss_Arena) - sizeof(Abyss_Arena);

    return arena;
}
abyss_handle abyss_arena_alloc(Abyss_Arena* arena, size_t size)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&arena->lock); });

    if ( size == 0 ) {
        return abyss_handle_switch(
            ((char*)arena) + arena->offset, arena->offset);
    }
    if ( arena->size - arena->offset < size ) {
        _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&arena->lock); });
        return abyss_handle_NULL;
    }

    abyss_handle hdl = abyss_handle_switch(
        &arena->buf[arena->offset], arena->offset + sizeof(Abyss_Arena));
    arena->offset = _ABYSS_ROUND_UP(arena->offset + size, ABYSS_DATA_ALIGN);

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&arena->lock); });

    return hdl;
}
void abyss_arena_free(Abyss_Arena* arena, abyss_handle hdl)
{
    _ABYSS_SAFETY_CHECK({
        if ( abyss_handle_is_NULL(hdl) ) { return; }

        _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&arena->lock); });

        if ( !_abyss_arena_contains(arena, abyss_handle_get_ptr(arena, hdl)) ) {
            _ABYSS_WARN(
                "invalid free, out of the allocated block mem range, it could "
                "be a double free, or a pointer that wasn't allocated in this "
                "allocator.");
        }

        _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&arena->lock); });
    });
}
void abyss_arena_reset(Abyss_Arena* arena)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&arena->lock); });

    arena->offset = _ABYSS_ROUND_S(Abyss_Arena) - sizeof(Abyss_Arena);

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&arena->lock); });
}

/* surge allocator ***********************************************************/

// [ASurge |*** allocated ***|     free     ]
// AS.buf-/                 /              /
// AS.buf[AS.off]----------/              /
// AS.buf[AS.size]-----------------------/
// if AS.count reaches 0 it will automatically reset the allocator
struct Abyss_Surge_s {
    _ABYSS_ADD_LOCK(lock);
    size_t size;
    size_t offset;
    size_t count;
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

    Abyss_Surge* surge = (Abyss_Surge*)buf;

    _ABYSS_THREAD_SAFE({
        if ( pthread_mutex_init(&surge->lock, NULL) != 0 ) {
            _ABYSS_WARN("mutex initialization failed");
            return NULL;
        }
    });

    surge->size = size - sizeof(Abyss_Surge);
    // align the buf with padding, by adding the offset
    surge->offset = _ABYSS_ROUND_UP(sizeof(Abyss_Arena), ABYSS_DATA_ALIGN)
        - sizeof(Abyss_Arena);
    surge->count = 0;

    return surge;
}
abyss_handle abyss_surge_alloc(Abyss_Surge* surge, size_t size)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&surge->lock); });
    if ( size == 0 ) {
        return abyss_handle_switch(
            (void*)(((char*)surge) + surge->offset), surge->offset);
    }
    if ( surge->size - surge->offset < size ) {
        _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&surge->lock); });
        return abyss_handle_NULL;
    }

    abyss_handle hdl = abyss_handle_switch(
        &surge->buf[surge->offset], surge->offset + sizeof(Abyss_Surge));
    surge->offset = _ABYSS_ROUND_UP(surge->offset + size, ABYSS_DATA_ALIGN);
    surge->count++;

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&surge->lock); });
    return hdl;
}
void abyss_surge_free(Abyss_Surge* surge, abyss_handle hdl)
{
    if ( abyss_handle_is_NULL(hdl) ) { return; }

    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&surge->lock); });

    _ABYSS_SAFETY_CHECK({
        if ( !_abyss_surge_contains(surge, abyss_handle_get_ptr(surge, hdl)) ) {
            _ABYSS_WARN(
                "invalid free, out of the allocated block mem range, it could "
                "be a double free, or a pointer that wasn't allocated in this "
                "allocator.");
            _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&surge->lock); });
            return;
        }

        if ( surge->count == 0 ) {
            _ABYSS_WARN("invalid double free");
            _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&surge->lock); });
            return;
        }
    });

    if ( surge->count == 1 ) {
        surge->offset = _ABYSS_ROUND_S(Abyss_Surge) - sizeof(Abyss_Arena);
    }
    surge->count--;

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&surge->lock); });
}
void abyss_surge_reset(Abyss_Surge* surge)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&surge->lock); });

    surge->offset = _ABYSS_ROUND_S(Abyss_Surge) - sizeof(Abyss_Arena);
    surge->count = 0;

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&surge->lock); });
}

/* allocator totem ***********************************************************/

// [ATotem ****** |A0|A1|A3|...|AN|XX|XX|XX]
// AT.allocators-/            /           /
// AT.allocators[AT.head]----/           /
// AT.allocators[AT.capacity]-----------/
struct Abyss_Totem_s {
    _ABYSS_ADD_LOCK(lock);
    unsigned short head;     // 4B + 4B vs 8B (size_t capacity; + NULL ptr)
    unsigned short capacity; // max cap: 2^16 - 1, (-1 in head is reserved)
    struct {
        void* ptr;
        abyss_allocator_t type;
    } allocators[];
};

Abyss_Totem* abyss_totem_init(void* buf, size_t size)
{
    _ABYSS_SAFETY_CHECK({
        if ( size < sizeof(Abyss_Totem) ) {
            _ABYSS_WARN("not enough mem provided for the totem struct...");
            return NULL;
        }
    });

    Abyss_Totem* totem = (Abyss_Totem*)buf;

    _ABYSS_THREAD_SAFE({
        if ( pthread_mutex_init(&totem->lock, NULL) != 0 ) {
            _ABYSS_WARN("mutex initialization failed");
            return NULL;
        }
    });

    totem->capacity = (size - sizeof(Abyss_Totem)) / sizeof(abyss_handle);
    totem->head = (unsigned short)-1;

    return totem;
}
abyss_handle abyss_totem_alloc(Abyss_Totem* totem, size_t size)
{
    abyss_handle hdl = abyss_handle_NULL;

    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&totem->lock); });

    for ( int i = totem->head; 0 <= i; i-- ) {
        switch ( totem->allocators[i].type ) {
            case ABYSS_ARENA_T:
                hdl = abyss_arena_alloc(
                    (Abyss_Arena*)totem->allocators[i].ptr, size);
                break;
            case ABYSS_SURGE_T:
                hdl = abyss_surge_alloc(
                    (Abyss_Surge*)totem->allocators[i].ptr, size);
                break;
            case ABYSS_TOTEM_T:
                hdl = abyss_totem_alloc(
                    (Abyss_Totem*)totem->allocators[i].ptr, size);
                break;
        }
        if ( abyss_handle_is_NULL(hdl) ) { break; }
    }

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&totem->lock); });

    return hdl;
}

inline static int _abyss_totem_free(Abyss_Totem* totem, void* ptr)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&totem->lock); });

    // test this shit :)
    for ( int i = totem->head; 0 <= i; i-- ) {
        switch ( totem->allocators[i].type ) {
            // FIX: if we are in handle mode this shit doesn't work, we need to
            //      ask the user for a dereferenced ptr...
            case ABYSS_ARENA_T: {
                Abyss_Arena* arena = (Abyss_Arena*)totem->allocators[i].ptr;
                if ( _abyss_arena_contains(arena, ptr) ) {
                    abyss_handle hdl = abyss_handle_get_hdl(arena, ptr);
                    abyss_arena_free(arena, hdl);
                    goto allocator_found;
                }
                break;
            }
            case ABYSS_SURGE_T: {
                Abyss_Surge* surge = (Abyss_Surge*)totem->allocators[i].ptr;
                if ( _abyss_surge_contains(surge, ptr) ) {
                    abyss_handle hdl = abyss_handle_get_hdl(surge, ptr);
                    abyss_surge_free(surge, hdl);
                    goto allocator_found;
                }
                break;
            }
            case ABYSS_TOTEM_T: {
                if ( _abyss_totem_free(
                         (Abyss_Totem*)totem->allocators[i].ptr, ptr) ) {
                    goto allocator_found;
                }
                break;
            }
        }
    }
    // allocator_not_found:
    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&totem->lock); });
    return 0;
allocator_found:
    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&totem->lock); });
    return 1;
}
void abyss_totem_free(Abyss_Totem* totem, void* ptr)
{
    // NOTE: _abyss_totem_free(...) returns a non 0 if the allocator was found
    //       inside the totem struct, and the respective free was called to the
    //       allocator.
    const int error = _abyss_totem_free(totem, ptr);
    _ABYSS_SAFETY_CHECK({
        if ( error ) {
            _ABYSS_WARN(
                "invalid free, the allocatos inside the totem didn't have it "
                "in range, the pointer that wasn't allocated in this "
                "allocator.");
        }
    });
}
void abyss_totem_reset(Abyss_Totem* totem)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&totem->lock); });

    for ( int i = totem->head; 0 <= i; i-- ) {
        switch ( totem->allocators[i].type ) {
            case ABYSS_ARENA_T:
                abyss_arena_reset((Abyss_Arena*)totem->allocators[i].ptr);
                break;
            case ABYSS_SURGE_T:
                abyss_surge_reset((Abyss_Surge*)totem->allocators[i].ptr);
                break;
            case ABYSS_TOTEM_T:
                abyss_totem_reset((Abyss_Totem*)totem->allocators[i].ptr);
                break;
        }
    }

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&totem->lock); });
}

void* abyss_totem_push(
    Abyss_Totem* totem, void* allocator, abyss_allocator_t type)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&totem->lock); });
    _ABYSS_SAFETY_CHECK({
        if ( totem->capacity <= (unsigned short)(totem->head + 1) ) {
            _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&totem->lock); });
            return abyss_handle_NULL;
        }
    });

    totem->head++;
    totem->allocators[totem->head].ptr = allocator;
    totem->allocators[totem->head].type = type;

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&totem->lock); });
    return allocator;
}
void* abyss_totem_pop(Abyss_Totem* totem, int indx, int* type)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_lock(&totem->lock); });

    if ( indx < 0 ) { indx = totem->head + indx + 1; }

    _ABYSS_SAFETY_CHECK({
        if ( totem->capacity < indx || (short)totem->head < 0 ) {
            _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&totem->lock); });
            return NULL;
        }
    });

    void* allocator = totem->allocators[indx].ptr;
    if ( type != NULL ) { *type = totem->allocators[indx].type; }
    for ( int i = indx; i < totem->head; i++ ) {
        totem->allocators[i] = totem->allocators[i + 1];
    }
    totem->head--;

    _ABYSS_THREAD_SAFE({ pthread_mutex_unlock(&totem->lock); });
    return allocator;
}

/* destroy *******************************************************************/

#        ifdef ABYSS_THREAD_SAFE_MODE

void abyss_arena_destroy(Abyss_Arena* aa)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_destroy(&aa->lock); });
}
void abyss_surge_destroy(Abyss_Surge* surge)
{
    _ABYSS_THREAD_SAFE({ pthread_mutex_destroy(&surge->lock); });
}
void abyss_totem_destroy(Abyss_Totem* totem)
{
    _ABYSS_THREAD_SAFE({
        pthread_mutex_lock(&totem->lock);
        for ( int i = totem->head; 0 <= i; i-- ) {
            switch ( totem->allocators[i].type ) {
                case ABYSS_ARENA_T:
                    abyss_arena_destroy((Abyss_Arena*)totem->allocators[i].ptr);
                    break;
                case ABYSS_SURGE_T:
                    abyss_surge_destroy((Abyss_Surge*)totem->allocators[i].ptr);
                    break;
                case ABYSS_TOTEM_T:
                    abyss_totem_destroy((Abyss_Totem*)totem->allocators[i].ptr);
                    break;
                default: _ABYSS_WARN("unknown allocator type");
            }
        }
        pthread_mutex_unlock(&totem->lock);
        pthread_mutex_destroy(&totem->lock);
    });
}

#        endif // !ABYSS_THREAD_SAFE_MODE
#    endif     // !ABYSS_IMPLEMENTATION

/* private helper funcions ***************************************************/

int _abyss_arena_contains(Abyss_Arena* arena, void* ptr)
{
    intptr_t delta = (char*)arena->buf - (char*)ptr;
    return 0 <= delta && (size_t)delta < arena->offset;
}
int _abyss_surge_contains(Abyss_Surge* surge, void* ptr)
{
    intptr_t delta = (char*)surge->buf - (char*)ptr;
    return 0 <= delta && (size_t)delta < surge->offset;
}
int _abyss_totem_contains(Abyss_Totem* totem, void* ptr)
{
    for ( int i = totem->head; 0 <= i; i-- ) {
        switch ( totem->allocators[i].type ) {
            case ABYSS_ARENA_T: {
                Abyss_Arena* arena = (Abyss_Arena*)totem->allocators[i].ptr;
                if ( _abyss_arena_contains(arena, ptr) ) { return 1; }
            } break;
            case ABYSS_SURGE_T: {
                Abyss_Surge* surge = (Abyss_Surge*)totem->allocators[i].ptr;
                if ( _abyss_surge_contains(surge, ptr) ) { return 1; }
            } break;
            case ABYSS_TOTEM_T:
                if ( _abyss_totem_contains(totem, ptr) ) { return 1; }
                break;
            default: _ABYSS_WARN("unknown allocator type");
        }
    }
    return 0;
}

/*****************************************************************************/

#    ifdef ABYSS_STRIP_PREFIX /////////////////////////////////////////////////
#        undef ABYSS_STRIP_PREFIX

#        define allocator_t abyss_allocator_t


#        define handle         abyss_handle
#        define handle_NULL    abyss_handle_NULL
#        define handle_cast    abyss_handle_cast
#        define handle_get_ptr abyss_handle_get_ptr
#        define handle_is_NULL abyss_handle_is_NULL
#        define handle_rel     abyss_handle_rel
#        define handle_switch  abyss_handle_switch

#        define ARENA_T     ABYSS_ARENA_T
#        define Arena       Abyss_Arena
#        define arena_alloc abyss_arena_alloc
#        define arena_free  abyss_arena_free
#        define arena_init  abyss_arena_init
#        define arena_reset abyss_arena_reset

#        define SURGE_T     ABYSS_SURGE_T
#        define Surge       Abyss_Surge
#        define surge_alloc abyss_surge_alloc
#        define surge_free  abyss_surge_free
#        define surge_init  abyss_surge_init
#        define surge_reset abyss_surge_reset

#        define TOTEM_SIZE  ABYSS_TOTEM_SIZE
#        define TOTEM_T     ABYSS_TOTEM_T
#        define Totem       Abyss_Totem
#        define totem_alloc abyss_totem_alloc
#        define totem_free  abyss_totem_free
#        define totem_init  abyss_totem_init
#        define totem_pop   abyss_totem_pop
#        define totem_push  abyss_totem_push
#        define totem_reset abyss_totem_reset

#        ifdef ABYSS_THREAD_SAFE_MODE
#            define arena_destroy abyss_arena_destroy
#            define surge_destroy abyss_surge_destroy
#            define totem_destroy abyss_totem_destroy
#        endif // !ABYSS_THREAD_SAFE_MODE

#    endif     // !ABYSS_STRIP_PREFIX

// cleanup
#    undef ABYSS_THREAD_SAFE_MODE
#    undef _ABYSS_WARN
#    undef _ABYSS_2WARN

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
