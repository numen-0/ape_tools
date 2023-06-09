
#include <stdio.h>

#include "inc/abyss.h"



int main(int argc, char *argv[])
{
    #ifdef DEBUG
    printf("[START] (debug_mode)\n");
    #else
    printf("[START]\n");
    #endif
    
    char * str;
    for (int i = 0; i < 9; i++) {
        str = malloc(sizeof(*str) * 2*i);

        if ( i % 2 )
            free(str);
    }
    abyss_print();
    str = realloc(str,sizeof(char *)*330);
    abyss_print();
    str = calloc(7,sizeof(char *));

    #ifdef DEBUG
    printf("[FINISH] (debug_mode)\n");
    abyss_print(); 
    #else
    printf("[FINISH]\n");
    #endif
    return 0;
}
