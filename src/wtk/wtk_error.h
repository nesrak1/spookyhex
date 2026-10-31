#ifndef WTK_ERROR_H
#define WTK_ERROR_H

#include "wtk_common.h"

WTK_NORET void wtk_throw(char* reason);
WTK_NORET void wtk_throwf(char* format, ...);

#endif // WTK_ERROR_H