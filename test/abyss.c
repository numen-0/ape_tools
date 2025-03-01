#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ALLOC_TEST_COUNT 100000

#define ABYSS_IMPLEMENTATION
#define ABYSS_STRIP_PREFIX
#define ABYSS_THREAD_SAFE_MODE
#include "../libs/abyss.h"

typedef struct {
    struct timespec start, end;
} Timer;

inline static void start_timer(Timer* t)
{
    clock_gettime(CLOCK_MONOTONIC, &t->start);
}

inline static double end_timer(Timer* t)
{
    clock_gettime(CLOCK_MONOTONIC, &t->end);
    return (t->end.tv_sec - t->start.tv_sec)
        + (t->end.tv_nsec - t->start.tv_nsec) / 1e9;
}

typedef struct {
    void* (*init)(void* buf, size_t size);
    void* (*alloc)(void* allocator, size_t size);
    void (*free)(void* allocator, void* ptr);
    void (*reset)(void* allocator);
    // debug
    size_t (*fmem)(void* allocator);
} Allocator_F;


static void shuffle(void* array[], size_t n) {
    if (n <= 1) return;

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        
        void* temp = array[i]; // swap
        array[i] = array[j];
        array[j] = temp;
    }
}

#define TEST_REPEAT_N (256 * 256)
#define TEST_POINTER_N   (256 * 4)
void test_allocator(const Allocator_F af, char buf[], size_t size)
{
    Timer t;
    void* allocator;

    { // init test
        double total = 0;
        for ( int i = 0; i < TEST_REPEAT_N; i++ ) {
            start_timer(&t);
            allocator = af.init(buf, size);
            total += end_timer(&t);
        }
        // total = total / TEST_REPEAT_N;
        size_t fmem = af.fmem(allocator);

        printf("    init  time:             %f sec.\n", total);
        // NOTE: this is avaible mem/total
        printf("    mem. efficiency:        %f%% (%zu/%zu)\n",
            ((double)fmem) * 100.0 / ((double)size), fmem, size);
    }

    { // alloc and free test
        // NOTE: make more test checking differen block sizes (all same or
        //       different size, with small or big blocks), with diffent
        //       number of allocations (a), with diffent number of frees (f <=
        //       n), with diffent order of freeing the pointers FIFO, LIFO or
        //       random
        double total = 0;
        void* arr[TEST_POINTER_N];

        for ( int i = 0; i < TEST_POINTER_N; i++ ) { // alloc 8B blocks
            start_timer(&t);
            void* ptr = af.alloc(allocator, 8);
            total += end_timer(&t);
            arr[i] = ptr;
        }
        // const double t_alloc = total / TEST_POINTER_N;
        const double t_alloc = total;

        total = 0;
        // srand(time(NULL));
        srand(42);
        shuffle(arr, TEST_POINTER_N);
        for ( int i = 0; i < TEST_POINTER_N; i++ ) { // free in random order
            void* ptr = arr[i];
            start_timer(&t);
            af.free(allocator, ptr);
            total += end_timer(&t);
            // IDEA: check the free percentage
        }
        // const double t_free = total / TEST_POINTER_N;
        const double t_free = total;

        const double t_non_empty = total / TEST_REPEAT_N;
        printf("    alloc time:             %f sec.\n", t_alloc);
        printf("    free  time:             %f sec.\n", t_free);
    }

    { // reset
        double total = 0;
        for ( int i = 0; i < TEST_REPEAT_N; i++ ) {
            start_timer(&t);
            af.reset(allocator);
            total += end_timer(&t);
        }
        // const double t_empty = total / TEST_REPEAT_N;
        const double t_empty = total;
        total = 0;
        for ( int i = 0; i < TEST_REPEAT_N; i++ ) {
            for ( int i = 0; i < size; i += sizeof(int) * 4 ) {
                af.alloc(allocator, sizeof(int));
            }
            start_timer(&t);
            af.reset(allocator);
            total += end_timer(&t);
        }
        // const double t_non_empty = total / TEST_REPEAT_N;
        const double t_non_empty = total;
        total = 0;

        printf("    reset time empty:       %f sec.\n", t_empty);
        printf("    reset time non empty:   %f sec.\n", t_non_empty);
    }
}
void test_arena_allocator()
{
    char buffer[1024 * 1024]; // 1MB arena
    Timer t;

    start_timer(&t);
    Abyss_Arena* arena = abyss_arena_init(buffer, sizeof(buffer));
    double init_time = end_timer(&t);

    printf("Arena init time: %f sec\n", init_time);

    // Test allocation performance
    void* ptrs[ALLOC_TEST_COUNT];

    start_timer(&t);
    for ( int i = 0; i < ALLOC_TEST_COUNT; i++ ) {
        ptrs[i] = abyss_arena_alloc(arena, 16); // 16-byte allocations
    }
    double alloc_time = end_timer(&t);

    printf(
        "Arena alloc time (%d x 16B): %f sec\n", ALLOC_TEST_COUNT, alloc_time);

    // Reset test
    start_timer(&t);
    abyss_arena_reset(arena);
    double reset_time = end_timer(&t);
    printf("Arena reset time: %f sec\n", reset_time);
}

///////////////////////////////////////////////////////////////////////////////

size_t arena_size(Abyss_Arena* aa) { return aa->size; }
size_t arena_free_mem(Abyss_Arena* aa) { return aa->size - aa->offset; }
size_t surge_size(Abyss_Surge* as) { return as->size; }
size_t surge_free_mem(Abyss_Surge* as) { return as->size - as->offset; }

///////////////////////////////////////////////////////////////////////////////

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
int main()
{
    // TODO: compare it with malloc and free :)
    size_t sizes[] = {
        256,
        1024,
        2048,
        8192,
    };

    struct {
        const char* name;
        const Allocator_F af;
    } allocators[] = {
        {
            .name = "Arena",
            .af = {
                .init = (void* (*)(void*, size_t))arena_init,
                .alloc = (void* (*)(void*, size_t))arena_alloc,
                .free = (void (*)(void*, void*))arena_free,
                .reset = (void (*)(void*))arena_reset,
                .fmem = (size_t (*)(void*))arena_free_mem,
            },
        }, {
            .name = "Surge",
            .af = {
                .init = (void* (*)(void*, size_t))surge_init,
                .alloc = (void* (*)(void*, size_t))surge_alloc,
                .free = (void (*)(void*, void*))surge_free,
                .reset = (void (*)(void*))surge_reset,
                .fmem = (size_t (*)(void*))surge_free_mem,
            }
        },
    };

    for ( int i = 0; i < ARRAY_LEN(allocators); i++ ) {
        printf("test: testing '%s' allocator\n", allocators[i].name);
        for ( int j = 0; j < ARRAY_LEN(sizes); j++ ) {
            const size_t size = sizes[j];
            char buf[size];
            printf("  buff size: %zu\n", size);
            test_allocator(allocators[i].af, buf, size);
        }
    }

    return 0;
}
