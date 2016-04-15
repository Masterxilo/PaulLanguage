#include <iostream>
#include <string>
using namespace std;
#include <Windows.h>
#include "assert.h"

#include "filepersist.h"
#include "ExpressionExt.h"
#include "Ruled.h"
#include "Parser.h"


#define E makeExpression
#define EMPTY makeEmpty()

void rep() {
    // Read
    cout << "|- ";
    string input;
    getline(cin, input);

    // Eval
    auto in = makeExpression(input);
    assert(in->symbol.length() >= input.length());
    auto out = //apply(makeExpression("cli", std::move(in)));
        stringify(apply(parseLax(std::move(in)))); // default

    //assert(in->symbol.length() == 0); // bam! -> in was moved

    // Print
    cout << out->symbol << endl; // TODO a sideeffect of cli could do this
}

void testExpressionAndPersist() {
    const char name[] = "h\0x";
    auto x = makeExpression(string(name, 3));
    assert(x->symbol.length() == 3);
    assert(x->symbol == string(name, 3));

    const char hello[] = "hello";
    x = makeExpression(hello);
    // old x will have deallocated here
    assert(x->symbol.length() == sizeof(hello) - 1);
    filePersist(x, hello);
    
    x = fileDepersist("hello");
    assert(!isEmpty(x));
    assert(x->symbol == "hello");

    x = E("");
    assert(isEmpty(x));
}

void addRule(string what, SPExpression to) {
    
    addRule(E("addRule", E(what), std::move(to)));
}

void testRuled() {
    auto r = getRules(EMPTY);
    auto q = getRules(EMPTY);
    assert(r != q); // reading multiple times gives copies, not the real thing
    const auto ct = r->subexpressions.size();
    //assert(r->subexpressions.size() == 0); // this will only be true when the system has no rules yet -- maybe rules should be split into multiple files
    assert(r->subexpressions.size() >= 0);
    
    r = getRules(EMPTY);
    assert(r->subexpressions.size() == ct);

    {
        vector < SPExpression > a_b;
        auto rr = addRule(E("addRule", E("a"), E("b")));
        assert(isEmpty(rr));
    }

    r = getRules(EMPTY);
    assert(r->subexpressions.size() == ct + 1 || r->subexpressions.size() == ct); // rule might already be present

    {
        auto r = apply(E("a"));
        assert(!isEmpty(r));
        assert(r.get()->symbol == "b");
    }
}

void testParser() {
    {
        auto r = parse(E("hello()"));
        assert(r.get()->symbol == "hello");
        assert(r.get()->subexpressions.size() == 0);
    }
    
    {
        auto r = parse(E("hello(world())"));
        assert(r->symbol == "hello");
        assert(r->subexpressions.size() == 1);
        assert(r->subexpressions[0]->symbol == "world");
        assert(r->subexpressions[0]->subexpressions.size() == 0);
 
        auto q = stringify(std::move(r));
        assert(q.get()->symbol == "hello(world() )", "%s", q.get()->symbol.c_str());
    }
    
    {
        auto r = parse(E("hello(world())"));
        assert(r->symbol == "hello");
        assert(r->subexpressions.size() == 1);
        assert(r->subexpressions[0]->symbol == "world");


        vector < SPExpression > b_c;
        b_c.push_back(E("b"));
        b_c.push_back(E("c"));
        auto q = stringify(E("a", std::move(b_c)));
        assert(q.get()->symbol == "a(b() c() )", "%s", q.get()->symbol.c_str());

        q = stringify(apply(parse(std::move(q))));

        // rule(a b) added in the rules test causes everything inside to be lost
        assert(q.get()->symbol == "b()", "%s", q.get()->symbol.c_str());
    }
}

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>


SPExpression parseStr(string what) {
    return parse(E(what));
}

void applyStr(string what) {
    apply(parse(E(what)));
}

void testBasicTools() {
    //S(parseStr("addBT(yes)"));
}

void testParseLax() {
    auto e = parseLax(E("f"));
    assert(e->symbol == "f");
    assert(e->subexpressions.size() == 0);

    auto f = deepCopy(e);
    assert(f != e);
    assert(f->symbol == "f");
    assert(f->subexpressions.size() == 0);


    e = parseLax(E("map(f g)"));
    assert(e->symbol == "map");
    assert(e->subexpressions.size() == 2);
    assert(e->subexpressions[0]->symbol == "f");
    assert(e->subexpressions[0]->subexpressions.size() == 0);
    assert(e->subexpressions[1]->symbol == "g");
    assert(e->subexpressions[1]->subexpressions.size() == 0);


    e = parseLax(E("map(f g(1 2))"));
    assert(e->symbol == "map");
    assert(e->subexpressions.size() == 2);
    assert(e->subexpressions[0]->symbol == "f");
    assert(e->subexpressions[0]->subexpressions.size() == 0);
    assert(e->subexpressions[1]->symbol == "g");
    assert(e->subexpressions[1]->subexpressions.size() == 2);
    assert(e->subexpressions[1]->subexpressions[0]->symbol == "1");
    assert(e->subexpressions[1]->subexpressions[1]->symbol == "2");


    e = deepCopy(e);
    assert(e->symbol == "map");
    assert(e->subexpressions.size() == 2);
    assert(e->subexpressions[0]->symbol == "f");
    assert(e->subexpressions[0]->subexpressions.size() == 0);
    assert(e->subexpressions[1]->symbol == "g");
    assert(e->subexpressions[1]->subexpressions.size() == 2);
    assert(e->subexpressions[1]->subexpressions[0]->symbol == "1");
    assert(e->subexpressions[1]->subexpressions[1]->symbol == "2");
}

int main() {
    auto e = E("test");
    //persist(hWritePipe, e.get());


    HANDLE               hReadPipe;
    HANDLE               hWritePipe;
    CreatePipe(&hReadPipe, &hWritePipe , 0, 0);
    WriteFile(hWritePipe, "hello", 5, 0, 0);

    int sz = GetFileSize(hReadPipe, 0);
    assert(sz == 5);
    char out[5];
    ReadFile(hReadPipe, out, 5, 0,0);

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | 
_CRTDBG_CHECK_ALWAYS_DF);

    testExpressionAndPersist();
    testRuled();
    testParser();
    testParseLax();
    testBasicTools();

    addRule("addRule", parseStr("_call(Ruled.dll() addRule() arg(0()) )"));
    //applyStr("_call(Ruled.dll() addRule() addRule(addRule _call(Ruled.dll() addRule() ) )")
    while (1) rep();
}