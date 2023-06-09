
#include <stdio.h>

#include "inc/timekeeper.h"

void f0()
{
    int j = 0;
    for ( int i = 0; i < 1000; i++ )
        j++;
}

void f1()
{
    int j = 0;
    for ( int i = 0; i < 100000; i++ )
        j++;
}

int main(int argc, char *argv[])
{
    printf("[START]\n");

    printf("f0 : %fs\n",timekeeper_benchmark_funct(&f0));
    printf("f1 : %fs\n",timekeeper_benchmark_funct(&f1));

    printf("[FINISH]\n");
    return 0;
}
