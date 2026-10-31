// stdint.h doesn't support stdbool.h, so here's a
// quick and dirty version for it. and no, _Bool did
// not exist so we have to typedef it to int :|

#ifndef _MSC_VER // [
#error "Use this header only with Microsoft Visual C++ compilers!"
#endif // _MSC_VER ]

#ifndef _MSC_STDBOOL_H_ // [
#define _MSC_STDBOOL_H_

#if _MSC_VER > 1000
#pragma once
#endif

#if _MSC_VER >= 1800 // [
#include <stdbool.h>
#else // ] _MSC_VER >= 1800 [

#ifndef __cplusplus

typedef int bool;
#define false 0
#define true 1

#endif // __cplusplus

#endif // _MSC_VER >= 1800 ]

#endif // _MSC_STDINT_H_ ]
