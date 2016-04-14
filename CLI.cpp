#include <iostream>
#include <string>
using namespace std;
#include <Windows.h>
#include "assert.h"

#include "ExpressionExt.h"
#include "Ruled.h"
#include "Parser.h"

bool rep() {
    // Read
    string input;
    getline(cin, input);

    // Eval
    PExpression in = makeExpression(input);
    validate(in);
    assert(getSymbol(in).length() >= input.length());
    PExpression out = stringify(apply(parse(in)));
    validate(out);

    // Print
    printf("%.*s\n", out->symbol_len, out->symbol);

    // Exit
    bool exit = getSymbol(out) == "exit";
    deallocate(out);
    return exit;
}

#define E makeExpression
#define S scope
#define EMPTY makeEmpty()
void testExpression() {
    auto x = makeExpression("hello");
    filePersist(x, "hello");
    deallocate(x);
    //validate(x); // bang! -- ok
    
    x = fileLoad("hello");
    assert(!isEmpty(x));
    validate(x);
    assert(getSymbol(x) == "hello");
    deallocate(x);

    x = E("");
    assert(isEmpty(x));
    deallocate(x);
}

void testRuled() {
    auto r = getRules(EMPTY);
    auto q = getRules(EMPTY);
    assert(r != q); // reading multiple times gives copies, not the real thing
    const auto ct = r->subexpression_count;
    //assert(r->subexpression_count == 0); // this will only be true when the system has no rules yet -- maybe rules should be split into multiple files
    assert(r->subexpression_count >= 0);
    assert(r->subexpression_count != INVALID_LENGTH);

    deallocate(r);
    deallocate(q);

    r = getRules(EMPTY);
    assert(r->subexpression_count == ct);
    deallocate(r);

    {
        auto r = S(addRule(E("addRule", {E("a"), E("b")})));
        assert(isEmpty(r.get()));
    }

    r = getRules(EMPTY);
    assert(r->subexpression_count == ct + 1);
    deallocate(r);

    {
        auto aa = E("a");
        auto r = S( apply(aa));
        //assert(aa->symbol_len == INVALID_LENGTH); // should now be deleted // yes, but it is reused right away
        assert(!isEmpty(r.get()));
        assert(getSymbol(r.get()) == "b");
    }
}

void testParser() {
    {
        auto r = S(parse(E("hello")));
        assert(getSymbol(r.get()) == "hello");
    }

    {
        auto r = parse(E("hello(world)"));
        assert(getSymbol(r) == "hello");
        assert(r->subexpression_count == 1);
        assert(getSymbol(r->subexpressions[0]) == "world");
 
        auto q = S(stringify(r));
        assert(getSymbol(q.get()) == "hello(world)");
    }
}
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

void addRule(string what, PExpression to) {
    S(addRule(E("addRule", {E(what), to})));
}

PExpression parseStr(string what) {
    return apply(parse(E(what)));
}

int main() {

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | 
_CRTDBG_CHECK_ALWAYS_DF);

    const int ca = countAllocated();
    testExpression();
    testRuled();
    testParser();
    //assert(ca == countAllocated(), "%d %d", ca,countAllocated()); // not true, we add new rules that we don't remove -- others?

    //assert(countAllocated() == 0, "%d", countAllocated());
    //S( // avoid leaks
    //    addRule(parseStr("addRule(addRule _call(Ruled.dll addRule arg(0))"))
    //    );
    while (!rep());
    //assert(countAllocated() == 0, "%d", countAllocated());
}