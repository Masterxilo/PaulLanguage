#pragma once
#include <TransformationLibrary.h>

/*++

Transformation Library Name:

    Ruled.h

Abstract:

    Implements a basic applicative system, capable of calling TransformationLibrary dlls.

    * Maintains an Expression "rules" of format
        rules(
            rule(a() _call(something.dll someTrafo arg(0))

            rule(c() d(arg(0) arg(1)))
        )
    * apply(f(...))
        * takes the symbol of its single subexpression
        * looks for a rule in rules with the same first symbol
        * if found, makes the result a copy of the right hand side of the rule with
            arg(0) replaced with f(...)
            arg(i), i > 1 replaced with the i-th subexpression of f.

            arg may only occur with a single argument which must be a decimal digit string representing a positive number.
            an assertion fails when f does not have enough children to serve an arg(i) request.

            TODO the argument replacement part could be moved to another library and decoupled from the replacement process, maybe inserting on the right hand side only arg(0) (_arg?)
        * Keeps doing this while any rule applies to the result.

    * getRules returns the current list of rules
    * addRule(sym() replacement) appends a rule
    * _call(dll.dll() functionName() parameter)
        Calls the tranformation "functionName" in the transformation library "dll.dll" passing parameter.

    This construction, given appropriate rules, allows apply to call transformations in any tranformation library:
    Suppose Lib.dll defines the transformation count(e).
    Then apply(count(e())) can be made to evaluate the function count on e by first defining this behaviour via

        addRule( count() _call(Lib.dll() count() arg(1)) )

Dependencies:

    Expression.dll, Persist.dll

Memory management & Thread safety:

    See TransformationLibrary.h

    Allocates rules when the dll is attached, frees it when detached.

Error handling:

    An assertion fails on invalid input.

--*/

#ifndef TRANSFORMATION_LIBRARY_COMPILE
#pragma comment(lib, "Ruled.lib")
#endif

/// Symbol must not have any children
/// \returns empty
DECL_TRANSFORMATION(addRule);// addRule(symbol expression)
DECL_TRANSFORMATION(getRules);

/// \returns result of transform function
DECL_TRANSFORMATION(_call); // PExpression call_dllfilename_TransformationName_arg
DECL_TRANSFORMATION(apply); // f(...), checks whether some rule(f ...) exists and applies it as long as possible
