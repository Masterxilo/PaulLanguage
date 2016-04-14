#include "Parser.h"
#include "ExpressionExt.h"
#include "assert.h"
#include <Windows.h>
#include <stdio.h>
#include <string>
#include <memory>
#include <iostream>
using namespace std;

#include <functional>
#include <string>
#include <vector>
using namespace std;
void skipspace(char *&s) { while (*s && isspace(*s))s++; }

string toString(PExpression e) {
    string s = getSymbol(e);
    assert(s.length() == e->symbol_len);
    if (!e->subexpression_count) return s;

    s += "(";
    const int n = e->subexpression_count;
    assert(n < 10 * 1000);
    eachSubexpressionIndexed(e, [&](PExpression& x, const int i) {
        s += toString(x);
        if (i < n - 1) s += " ";
    });
    s += ")";
    return s;
}

void eachSubexpressionIndexed_(PExpression e, function<void(PExpression&, const int)> f) {

}

void doit() {
    eachSubexpressionIndexed_(0, [&](PExpression& se, const int) {
    });
}


// Consumes initial and final space and
// [symbol]
// [symbol]    (<fromString> ... <fromString>)
// s must point to 0 or a 0-terminated string.
PExpression fromString(char*& s) {
    assert(*s);
    string symbol = "";
    vector<PExpression> subexpressions;

    skipspace(s);
    while (*s && !isspace(*s) && *s != '(' && *s != ')') {
        symbol += *s;
        s++;
    }
    skipspace(s);

    if (*s == '(') {
        s++;
        assert(*s);
        skipspace(s);
        assert(*s);
        while (*s && *s != ')') {
            auto e = fromString(s);
            subexpressions.push_back(e);
        }
        if (*s == ')') s++;
    }

    skipspace(s);

    return makeExpression(symbol, subexpressions);
}

extern "C" {
    PExpression stringify(PExpression e) {
        auto o = makeExpression(toString(e));
        deallocate(e);
        return o;
    }
    PExpression parse(PExpression symbol) {
        assert(symbol->subexpression_count == 0);

        string str = getSymbol(symbol);
        char* s = (char*)str.c_str();
        assert(*(s + symbol->symbol_len) == 0, "s must be zero terminated");
        auto o = fromString(s);

        deallocate(symbol);
        return o;
    }
}
