#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "wtk_error.h"

WTK_NORET void wtk_throw(char* reason)
{
    printf("exception thrown: %s\n", reason);
    exit(1);
}

WTK_NORET void wtk_throwf(char* format, ...)
{
    va_list arg;
    printf("exception thrown: ");
    va_start(arg, format);
    vprintf(format, arg);
    va_end(arg);
    printf("\n");
    exit(1);
}