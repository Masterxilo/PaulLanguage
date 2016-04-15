#pragma once

#include <vector>
#include <string>

/*++

Module Name:

    Expression.h

Abstract:

    * Defines the data structure Expression which shall be treated as if defined as follows:

        struct Expression {
            std::string symbol;
            std::vector<Expression*> subexpressions;
        };

        It stores a sequence of bytes and valid pointers to other Expressions.
    
    * Enforces some the constraints mentioned below and can check some. However, it is the users responsibility to ensure their validity is maintained.
    * Allocates and deeply deallocates such expressions.
    
    An Expression:
    * May not be allocated through means other than allocate(). Specifically, it cannot be:
        * copied (shallowly)
        * allocated on the stack
    * The set of all Expressions existing in the system at any given time is always a DAG, directed acyclic graph.

    Failure to ensure any of these constraints may cause a fatal failure at the time the structure is invalidated and may result in undefined behaviour of functions accepting such a PExpression (directly or indirectly). 

Dependencies:

    none

Memory management:

    Manages memory for all valid PExpressions currently existing.
    Does not free the memory when unloaded.

Thread safety:

    The functions are not synchronized.

    The user is responsible for ensuring Expressions are not freed multiple times from different threads and the user has to avoid data-races.

Error handling:

    An assertion fails on invalid input.
    No exceptions are thrown.

--*/

#ifdef EXPRESSION_DLL_COMPILE
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#pragma comment(lib, "Expression.lib")
#endif

struct Expression;
typedef Expression* PExpression;


/// Recursive i.e. deep delete.
/// No-op on 0, ignores subexpression[i] == 0.
/// Thus, deallocate(allocate(0,0)) is allowed.
API void deallocate(PExpression);


#include <memory>
#include <iostream>
#include <functional>
// Wrap and call deallocate as the pointer goes out of scope
typedef std::unique_ptr<Expression, std::function<void(PExpression)>> SPExpression;

inline void deallocateExpression(PExpression p) {
    //std::cout << "unique_ptr deallocating " << p << std::endl; 
    deallocate(p);
};
inline SPExpression scope(PExpression e) {
    return SPExpression(e, deallocateExpression);
}

API SPExpression allocate();

class Expression {
private:
    void operator=(Expression);
    Expression() {}
    Expression(Expression&);
    ~Expression() {}

    friend SPExpression allocate();
    friend void deallocate(PExpression);
public:
    std::string symbol;
    std::vector<SPExpression> subexpressions;
};

#undef API