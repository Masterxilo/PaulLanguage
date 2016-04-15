#pragma once

/*++

Module Name:

    ExpressionExt.h

Abstract:
    
    Dependency-free convenience methods for dealing with Expressions.

--*/

#include "Expression.h"
#include <functional>
#include "assert.h"
#include <string>
#include <Windows.h>
#include <vector>
using namespace std;

#include <memory>
#include <iostream>


inline bool hasSymbol(const SPExpression&  e, string symbol) {
    return e->symbol == symbol;
}

inline bool hasSameSymbol(const SPExpression& e, const SPExpression& f) {
    return hasSymbol(e, f->symbol);
}

inline bool validateExpression(const SPExpression&  e, string symbol, unsigned int children) {
    assert(e);
    return hasSymbol(e, symbol) && e->subexpressions.size() == children;
}

// _expsymbol
// for scoped pointers of DEF_TRANSFORMATION
#define INPUT_IS(_expsymbol, _expchildren) {string expsym = (_expsymbol); const unsigned int excpchildren = (_expchildren); Tassert(validateExpression(e, expsym, excpchildren), "input has wrong format, expected '%s' and %d children, got '%s' and %d children", expsym.c_str(), excpchildren, e->symbol.c_str(), e->subexpressions.size());}

// The caller is responsible for freeing the output.
// Input expressions are used by-pointer (i.e. the caller has to free them later indirectly).
inline SPExpression makeExpression(string symbol, vector<SPExpression> subs = vector<SPExpression>()) {
    for (auto& s : subs) assert(s);
    auto e = allocate();
    e->symbol = symbol;
    e->subexpressions = std::move(subs);
    return e;
}

inline SPExpression makeExpression(string symbol, SPExpression sub0 ) {
    assert(sub0);
    vector<SPExpression> subs;
    subs.push_back(std::move(sub0));
    return makeExpression(symbol, std::move(subs));
}


inline SPExpression makeExpression(string symbol, SPExpression sub0, SPExpression sub1) {
    assert(sub0); assert(sub1);
    vector<SPExpression> subs;
    subs.push_back(std::move(sub0));
    subs.push_back(std::move(sub1));
    return makeExpression(symbol, std::move(subs));
}

#define deepCopy(x) deepCopy_((x).get())
// The caller is responsible for freeing the input (and output)
inline SPExpression deepCopy_(PExpression e) {
    assert(e);
    auto o = allocate();
    o->symbol = e->symbol;

    for (const auto& s : e->subexpressions)
        o->subexpressions.push_back(deepCopy_(s.get()));

    assert(o.get() != e);
    return o;
}

// The caller is responsible for freeing the now detached expressions
// The then-emptied expression is freed
inline vector<SPExpression> getAndDetachEachSubexpressionAndDeallocate(SPExpression e) {
    auto val = std::move(e->subexpressions);
    assert(e->subexpressions.empty());
    // for ()   e->subexpressions[0].reset();
    //e->subexpressions.clear();
    return val;
}

inline bool isEmpty(const SPExpression&  x) {
    return x->symbol == "" && x->subexpressions.size() == 0;
}

// The caller is responsible for freeing the return value.
inline SPExpression makeEmpty() {
    auto x = makeExpression("");
    assert(isEmpty(x));
    return x;
}
