
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ABYSS_C
#include "../inc/abyss.h"

#define ARR_SIZE 1<<8
#define NULL_PTR 0

typedef struct {            /* 32 */
    void * address;         /*  8 */
    size_t size;            /*  8 */
    char * file_name;       /*  8 */
    uint32_t line_number;   /*  4 */
} Meta_Ptr;

typedef struct {
    uint32_t len;
    Meta_Ptr arr[ARR_SIZE];
} Meta_Ptr_Array;

static Meta_Ptr_Array unfreed_soul_logs;


void * abyss_calloc(size_t nitems, size_t size, char * file_name, uint32_t line_number)
{
    void * ptr = calloc(nitems, size);  

    if ( !ptr )
        return NULL_PTR;

    if ( unfreed_soul_logs.len == ARR_SIZE ) 
        return ptr;

    Meta_Ptr * new_entry = &unfreed_soul_logs.arr[unfreed_soul_logs.len++];    
    new_entry->address = ptr;
    new_entry->size = size*nitems;
    new_entry->file_name = file_name;
    new_entry->line_number = line_number;

    return ptr;
}

void * abyss_malloc(size_t size, char * file_name, uint32_t line_number)
{
    void * ptr = malloc(size); 

    if ( !ptr )
        return NULL_PTR;
    
    if ( unfreed_soul_logs.len == ARR_SIZE ) 
        return ptr;

    Meta_Ptr * new_entry = &unfreed_soul_logs.arr[unfreed_soul_logs.len++];    
    new_entry->address = ptr;
    new_entry->size = size;
    new_entry->file_name = file_name;
    new_entry->line_number = line_number;

    return ptr;
}

void * abyss_realloc(void * ptr, size_t size, char * file_name, uint32_t line_number)
{
    void * new_ptr = realloc(ptr, size);

    if ( !new_ptr ) return NULL_PTR;

    Meta_Ptr *entry, *last_entry;

    last_entry = &unfreed_soul_logs.arr[unfreed_soul_logs.len];

    for ( entry = &unfreed_soul_logs.arr[0]; entry != last_entry; entry++)
    {
        if ( entry->address == ptr ) {
            entry->address = new_ptr;
            entry->size = size;
            entry->file_name = file_name;
            entry->line_number = line_number;
            break;
        }
    }
    return new_ptr;
}

void abyss_free(void * ptr)
{
    Meta_Ptr *entry, *last_entry;

    last_entry = &unfreed_soul_logs.arr[unfreed_soul_logs.len];

    for ( entry = &unfreed_soul_logs.arr[0]; entry != last_entry; entry++)
    {
        if ( entry->address == ptr ) {
            free(ptr);

            *entry = *last_entry;
            unfreed_soul_logs.len--;
            return;
        }
    }
}

void abyss_print() 
{
    printf(" address        │ size(B) │ file             │ line\n");
    printf("────────────────┼─────────┼──────────────────┼──────────\n");

    size_t total = 0;
    Meta_Ptr *entry, *last_entry;

    last_entry = &unfreed_soul_logs.arr[unfreed_soul_logs.len];

    for ( entry = &unfreed_soul_logs.arr[0]; entry != last_entry; entry++)
    {
        printf(" %p │ %-7ld │ %-16s │ %-5d\n"
               ,entry->address,entry->size,entry->file_name,entry->line_number);
        total += entry->size;
    }
    printf("────────────────┴─────────┴──────────────────┴──────────\n");
    printf(" TOTAL :          %ld\n",total);
    printf("────────────────────────────────────────────────────────\n");
}

