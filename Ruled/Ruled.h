/**
Implements persistent rule-based transformations of expressions.


=== Apply and _call ===
When apply is called on an expression, its head is matched with known 
rule's heads. If they match perfectly, the head is replaced with the body of the rule, verbatim pasting any referenced arguments. Copies are made.
The arguments are called arg(1), arg(2), arg(3)..., arg(0) is the whole expression.
The result is passed to apply again -- a loop is used, so there are no stack limitations.

If no match is found, the expression is returned unchanged.

There is one built-in rule, _call, syntax  "_call(Parser.dll parse expr)"
which will call the TTransformation function found in the specified dll with the specified expression.

=== Passed parameters ===
Any PExpression passed to functions is non-null.
More precisely, validate(e) succeeds.

TODO for optimization purposes, we might want to relieve this.

=== Memory management ===
Transformations must make sure that the expression passed to them is either deallocated or reachable from the expression they return.

All of the functions in this library are transformations. 
They adhere to the contract, meaning that they ensure that things passed 
to them are freed at some point and that things they return can be freed.
*/


#pragma once
#include "Expression.h"
typedef PExpression (*TTransformation)(PExpression);

#ifdef RULED_DLL_COMPILE
#define API __declspec(dllexport)
#define API_IMPORT_ALWAYS __declspec(dllexport)
#else
#define API __declspec(dllimport) // TODO those could be dynamically used
#define API_IMPORT_ALWAYS __declspec(dllimport)
#pragma comment(lib, "Ruled.lib")
#endif

extern "C" {
    /// Symbol must not have any children
    /// \returns empty
    API PExpression addRule(PExpression addRule_symbol_expression);
    API PExpression getRules(PExpression);

    /// \returns result of transform function
    API PExpression _call(PExpression call_dllfilename_TransformationName_arg);
    API_IMPORT_ALWAYS PExpression apply(PExpression expr);
};