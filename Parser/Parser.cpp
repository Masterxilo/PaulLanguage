#include "Parser.h"
#include "assert.h"
#include <ctype.h>
using namespace std;


// [symbol] (toString ... toString )
string toString(PExpression e) {
    string s = e->symbol;

    s += "(";
    for (auto& x : e->subexpressions) {
        s += toString(x.get()) + " ";
    }
    s += ")";
    return s;
}

void skipspace(char *&s) { while (*s && isspace(*s))s++; }
// Consumes initial and final space and
// [symbol] (<fromString> ... <fromString>)
// s must point to 0 or a 0-terminated string.
SPExpression fromString(char*& s) {
    assert(*s);
    SPExpression e = allocate();

    skipspace(s);
    while (*s && !isspace(*s) && *s != '(' && *s != ')') {
        e->symbol += *s;
        s++;
    }
    skipspace(s);

    Tassert(*s == '(', "expected '(' found '%c'", *s == 0 ? '0' : *s);
    s++;
    Tassert(*s);
    skipspace(s);
    Tassert(*s);

    while (*s && *s != ')') {
        e->subexpressions.push_back(fromString(s));
    }

    Tassert(*s == ')');
    s++;

    skipspace(s);

    return e;
}

// Consumes initial and final space and
// [symbol] // with space afterwards
// [symbol](<fromString> ... <fromString>) // no space before brace
// s must point to 0 or a 0-terminated string.
// Superset of strict parsing
SPExpression fromStringLax(char*& s) {
    SPExpression e = allocate();
    Tassert(*s);

    skipspace(s);
    while (*s && !isspace(*s) && *s != '(' && *s != ')') {
        e->symbol += *s;
        s++;
    }
    bool brace = false;
    if (*s == '(') {
        brace = true;
        s++;
        Tassert(*s);
        skipspace(s);
        Tassert(*s);

        while (*s && *s != ')') {
            e->subexpressions.push_back(fromStringLax(s));
        }

        Tassert(*s == ')', "expected ')' found '%c' (missing closing braces)", *s == 0 ? '0' : *s);
        s++;
    }
    assert(brace || e->subexpressions.size() == 0);
    skipspace(s);
    return e;
}

DEF_TRANSFORMATION(stringify) {
    return makeExpression(toString(e.get()));
}

DEF_TRANSFORMATION(parse) {
    Tassert(e->subexpressions.empty());

    char* s = (char*)e->symbol.c_str();
    return fromString(s);
}

DEF_TRANSFORMATION(parseLax) {
    Tassert(e->subexpressions.empty());

    char* s = (char*)e->symbol.c_str();
    return fromStringLax(s);
}
