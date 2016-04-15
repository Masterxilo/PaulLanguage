#pragma once

/*++ 

Module Name:

    Persist.h

Abstract:

    Serializes and restores Expressions

Dependencies:

    Expression.dll

Memory management:

    All expressions are allocated by Expression.dll.
    The caller is responsible for requesting deallocation of expressions passed to this and returned from this.

Thread safety:

    The functions are not synchronized and use global storage.

Error handling:

    An assertion fails on invalid input.

--*/

#include <Windows.h>
#include <Expression.h>

#ifdef PERSIST_DLL_COMPILE
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#pragma comment(lib, "Persist.lib")
#endif

/// \returns makeEmpty(), possibly nested, when there is not enough data
API void persist(HANDLE hFile, const SPExpression&);
API SPExpression load(HANDLE hFile);

//#undef API