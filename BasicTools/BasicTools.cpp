#include "transformationLibrary.h"
#include "expression.h"
#include "expressionext.h"
#include "Ruled.h"
#include "filepersist.h"
#include "persist.h"

#pragma comment(lib, "Ruled.lib")

struct Pipe {
    HANDLE hReadPipe;
    HANDLE hWritePipe;
    ~Pipe() {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
    }
};

// data(...)
DEF_TRANSFORMATION(serialize) {
    Pipe pipe;
    Tassert(CreatePipe(&pipe.hReadPipe, &pipe.hWritePipe, 0, 0), "could not create pipe");

    // Persist
    persist(pipe.hWritePipe, e);

    // Read into string s
    size_t sz = GetFileSize(pipe.hReadPipe, 0);
    string s(sz, 0);
    assert(s.length() == sz);
    assert(sz >= e->symbol.length(), "serialized data should be longer than head's symbol");

    DWORD read;
    ReadFile(pipe.hReadPipe, (char*)s.data(), sz, &read, 0);
    assert(read == sz);

    return makeExpression(s);
}

// data()
DEF_TRANSFORMATION(deserialize) {
    Tassert(e->subexpressions.size() == 0);

    Pipe pipe;
    Tassert(CreatePipe(&pipe.hReadPipe, &pipe.hWritePipe, 0, 0), "could not create pipe");

    // Write data
    DWORD written;
    auto serialDataLen = e->symbol.length();
    WriteFile(pipe.hWritePipe, (char*)e->symbol.data(), serialDataLen, &written, 0);
    assert(written == serialDataLen);

    // load
    e = load(pipe.hReadPipe);
    assert(e->symbol.length() <= serialDataLen, "symbol of head should be smaller than entire serialized data"); 

    return e;
}


// filePersist(fn() data(...))
DEF_TRANSFORMATION(filePersist) {
    INPUT_IS("filePersist", 2);
    Tassert(e->subexpressions[0]->symbol.length() > 0);
    Tassert(e->subexpressions[0]->subexpressions.size() == 0);

    ::filePersist(e->subexpressions[1], e->subexpressions[0]->symbol.c_str());
    return makeEmpty();
}

// fn()
DEF_TRANSFORMATION(fileDepersist) {
    Tassert(e->symbol.length() > 0);
    Tassert(e->subexpressions.size() == 0);

    return fileDepersist(e->symbol.c_str());
}

void Read(HANDLE hFile, std::string& to);
void Write(HANDLE hFile, const std::string& what);

// Save flat data
// fileSave(fn() data())
DEF_TRANSFORMATION(fileSave) {
    INPUT_IS("fileSave", 2);
    Tassert(e->subexpressions[0]->symbol.length() > 0);
    Tassert(e->subexpressions[0]->subexpressions.size() == 0);

    Tassert(e->subexpressions[1]->subexpressions.size() == 0);

    auto hFile = CreateFile(e->subexpressions[0]->symbol.c_str(), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
    Tassert(hFile != INVALID_HANDLE_VALUE, "could not create file");
    Write(hFile, e->subexpressions[1]->symbol);
    CloseHandle(hFile);

    return makeEmpty();
}

// load flat data
// fn()
DEF_TRANSFORMATION(fileLoad) {
    Tassert(e->symbol.length() > 0);
    Tassert(e->subexpressions.size() == 0);

    auto hFile = CreateFile(e->symbol.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    Tassert(hFile != INVALID_HANDLE_VALUE, "could not open file");
    e->symbol.resize(GetFileSize(hFile, 0));
    Read(hFile, e->symbol);
    CloseHandle(hFile);
    return e;
}


DEF_TRANSFORMATION(yes) {
    return makeExpression("yes");
}

DEF_TRANSFORMATION(no) {
    return makeExpression("no");
}
    
DEF_TRANSFORMATION(equal) {
    INPUT_IS("equal", 2);
    Tassert(e->subexpressions[0]->subexpressions.size() == 0);
    Tassert(e->subexpressions[1]->subexpressions.size() == 0);
        
    auto o = hasSameSymbol(
        e->subexpressions[0], e->subexpressions[1]) ?
        makeExpression("yes") : makeExpression("no");
    return o;
}

// applyEach(f(...)) calls apply on each argument of f, then returns the resulting f
DEF_TRANSFORMATION(applyEach) {
    INPUT_IS("applyEach", 1);
    auto f = std::move(e->subexpressions[0]);
    for (auto& s : f->subexpressions) {
        s = apply(std::move(s));
    }
    return f;
}

// f(...) calls deepApplyEach on the children and then apply on itself
DEF_TRANSFORMATION(deepApplyEach) {
    if (hasSymbol(e, "holdOnce")) {
        Tassert(e->subexpressions.size() == 1);
        return std::move(e->subexpressions[0]);
    }
    if (hasSymbol(e, "hold")) return e;
    if (hasSymbol(e, "apply")) return apply(std::move(e));

    for (auto& s : e->subexpressions) {
        s = deepApplyEach(std::move(s));
    }
    return apply(std::move(e));
}



DEF_TRANSFORMATION(print) {
    cout << "print: [" << e->symbol << "]" << endl;
    return e;
}

#include <stack>
#include <tuple>
// Stringify as a tree each subexpression
DEF_TRANSFORMATION(tree) {
    stack < pair<int,PExpression> > s;
    s.push({0, e.get()});
    string out;
    while (!s.empty()) {
        pair<int, PExpression> le = s.top(); s.pop();

        auto l = le.first;
        auto x = le.second;
        out += string(l*4, ' ') + x->symbol + "\n";
        for (auto it = x->subexpressions.rbegin(); it != x->subexpressions.rend(); it++) {
            s.push({l + 1, it->get()});
        }
    }

    return makeExpression("\n"+out);
}


// map(g f(...)) prepends g to each argument of f
DEF_TRANSFORMATION(map) {
    INPUT_IS("map", 2);
    Tassert(e->subexpressions[0]->subexpressions.size() == 0);

    auto g = e->subexpressions[0]->symbol;

    auto f = std::move(e->subexpressions[1]);
    for (auto& s : f->subexpressions) {
        s = makeExpression(g, std::move(s));
    }
    return f;
}
    
DEF_TRANSFORMATION(replaceHead) {
    INPUT_IS("replaceHead", 2);
        

    auto a_exp = getAndDetachEachSubexpressionAndDeallocate(std::move(e));
    //assert(e.get() && e->symbol.length() >= 0); // bam!
    Tassert(a_exp[0]->subexpressions.size() == 0);

    return makeExpression(
        a_exp[0]->symbol,
        getAndDetachEachSubexpressionAndDeallocate(std::move(a_exp[1]))
        );
}

DEF_TRANSFORMATION(concat) {
    Tassert(hasSymbol(e, "concat"));

    string out;
    for (auto& s : e->subexpressions) {
        Tassert(s->subexpressions.size() == 0, "arguments should have no subexpressions");
        out += s->symbol;
    }
    return makeExpression(out);
}

DEF_TRANSFORMATION(head) {
    return makeExpression(e->symbol);
}

