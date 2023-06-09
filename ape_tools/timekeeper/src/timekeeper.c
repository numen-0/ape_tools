
#include <time.h>

#include "../inc/timekeeper.h"

// typedef return   (*FuncTypeName)(take);
// typedef void     (*functiontype)();
// typedef char     (*functiontype)(int);
typedef void (*Void_Funct_Void)();

double timekeeper_benchmark_funct(Void_Funct_Void funct)
{
    double startTime = (double)clock()/CLOCKS_PER_SEC;

    funct();

    double endTime = (double)clock()/CLOCKS_PER_SEC;

    return endTime - startTime;
}
