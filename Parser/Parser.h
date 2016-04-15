#pragma once
#include <TransformationLibrary.h>

/*++ 

Transformation Library Name:

    Parser.h

Abstract:

    Serializes and restores most Expressions into ASCII strings.

    It does not handle non-printable and whitespace characters on input and cannot output these characters correctly.
    On input, whitespace charaters are ignored. For output, all bytes of each symbol are reproduced verbatim.

    The input has the syntax

        E ::= symbol ( E _ E ...)

    where symbol is a sequence of non-space characters and _ denotes one or more required whitespace characters.

    E _ E ... repeats exactly  e->subexpression.size() times.
    Given 0 subexpressions, symbol() is produced.

    This is the syntax used in other transformation library headers to document the input expressions.
    Examples:

        hello()         -- represents the Expression with symbol == "hello" and no subexpressions
        f(g())
        (1() 2() 3())
        number(0(1()))

    A variant of this parser might not require braces for arguments with no subexpressions, or might specify arities to allow for more concise notation:

        + 1 2
    
    could be made to stand for
        
        +(1() 2())

Dependencies:

    Expression.dll

Memory management & Thread safety:

    See TransformationLibrary.h

Error handling:

    An assertion fails on invalid input.

--*/


#ifndef TRANSFORMATION_LIBRARY_COMPILE
#pragma comment(lib, "Parser.lib")
#endif

extern "C" {
    // symbol
    DECL_TRANSFORMATION(parse);
    DECL_TRANSFORMATION(parseLax);

    // symbol
    DECL_TRANSFORMATION(stringify);
};
