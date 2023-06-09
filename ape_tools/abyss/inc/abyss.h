#ifndef ABYSS_H
#define ABYSS_H

#ifdef DEBUG

#include <stdint.h>

extern void * abyss_calloc(size_t nitems, size_t size, char * file_name, uint32_t line_number);
extern void *  abyss_malloc(size_t size, char * file_name, uint32_t line_number);
extern void *  abyss_realloc(void * ptr, size_t size, char * file_name, uint32_t line_number);
extern void    abyss_free(void * ptr);
extern void    abyss_print();

#ifndef ABYSS_C
#define calloc(nitems,size) abyss_calloc(nitems, size, __FILE__, __LINE__)
#define malloc(size)        abyss_malloc(size, __FILE__, __LINE__)
#define realloc(ptr,size)   abyss_realloc(ptr, size, __FILE__, __LINE__)
#define free(ptr)           abyss_free(ptr)
#endif // !ABISS_C

#endif // DEBUG

#endif // !ABYSS_H
