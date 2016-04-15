#pragma once

/*++

Included by each Transformation Library

Abstract:
    
    A Transformation library is a dll of extern "C" declared functions of type TTransformation.
    The header only serves to document each TL -- it can be used directly with LoadLibrary and GetProcAddress, no need to use the linker .lib generated with the dll.

    For convenience, tranformations are declared to take an empty expression when used without arguments. 

    Use Tassert in implementations to deal with erroneous usage.

Dependencies:

    Expression.dll

Memory management: 

    Transformations must make sure that each expression (indirectly) passed to them is either deallocated or reachable from the expression they return.

Thread safety:

    transformations are not required to be threadsafe

Error handling:

    An assertion may fail on invalid input.
    Transformation libraries may also define their own policies for error handling, e.g. returning special or unchanged values.
    No exceptions are thrown.

--*/

#include <Expression.h>
#include <ExpressionExt.h>

/**
Transformations must make sure that each expression (indirectly) passed to them is either deallocated or reachable from the expression they return.

This can be ensured by using the DEF_TRANSFORMATION macro, together with DECL_TRANSFORMATION and ExpressionExt.h.

Use .release when you return the expression unchanged.
*/
typedef SPExpression(*TTransformation)(SPExpression);


#ifdef TRANSFORMATION_LIBRARY_COMPILE
#define TRANSFORMATION __declspec(dllexport)
#else
#define TRANSFORMATION __declspec(dllimport)
#endif

#define DECL_TRANSFORMATION(name) extern "C" TRANSFORMATION SPExpression name(SPExpression e = makeEmpty())
#define DEF_TRANSFORMATION(name) extern "C" TRANSFORMATION SPExpression name(SPExpression e)

/// Use for user-errors, not programming errors.
/// assert for use within a DEF_TRANSFORMATION defined function. Returns an assertionError(<str>) where <str> is constructed with sprintf.
#define Tassert(x,commentFormat,...) {static_assert(std::is_same<decltype(e), SPExpression>::value, "Tassert must be used within a DEF_TRANSFORMATION definition"); if (!(x)) {char _s[1000 /*not too much or heap*/]; sprintf_s(_s, "\n===\n%s(%i) : Transform Assertion failed : %s.\n\t<" commentFormat ">\ne:\n\t%.*s(<%d>)\n===\n", __FILE__, __LINE__, #x, __VA_ARGS__, e ? e->symbol.length() : sizeof("<already deleted>"), e ? e->symbol.c_str() : "<already deleted>", e ? e->subexpressions.size() : 0);  OutputDebugStringA(_s); puts(_s); /*DebugBreak(); */return makeExpression("assertionError", makeExpression(_s)); } } 
