#include "Expression.h"
#include <functional>
#include "assert.h"
#include <string>
#include <Windows.h>
#include <vector>
using namespace std;

#include <memory>
#include <iostream>


auto deallocate_ = [](PExpression p) { std::cout << "unique_ptr deallocating " << p << endl; };
// Wrap and call deallocate as the pointer goes out of scope
std::unique_ptr<Expression, decltype(deallocate_)> scope(PExpression e) {
    return std::unique_ptr<Expression, decltype(deallocate_)>(e, deallocate_);
}

void eachSubexpressionIndexed(PExpression e, function<void(PExpression&, const int)> f) {
    validate(e);
    for (unsigned int i = 0; i < e->subexpression_count; i++) {
        validate(e->subexpressions[i]);
        f(e->subexpressions[i], i);
        validate(e->subexpressions[i]);
    }
    validate(e);
}

void eachSubexpression(PExpression e, function<void(PExpression&)> f) {
    eachSubexpressionIndexed(e, [&](PExpression& se, const int) {f(se); });
}

void eachSubexpressionInitIndexed(PExpression e, function<void(PExpression&, const int)> f, bool allowLosingSubexpressions = false) {
    for (unsigned int i = 0; i < e->subexpression_count; i++) {
        if (!allowLosingSubexpressions) assert(e->subexpressions[i] == 0);
        f(e->subexpressions[i], i);
    }
    if (!allowLosingSubexpressions) validate(e);
}

void eachSubexpressionInit(PExpression e, function<void(PExpression&)> f, bool allowLosingSubexpressions = false) {
    eachSubexpressionInitIndexed(e, [&](PExpression& se, const int) {f(se); }, allowLosingSubexpressions);
}

inline bool hasSymbol(PExpression e, string symbol) {
    validate(e);
    return (e->symbol_len == symbol.length()) &&
        memcmp(e->symbol, symbol.c_str(), e->symbol_len) == 0;
}

// Ensures that an expression has a certain string symbol.
inline void validateExpression(PExpression e, string symbol) {
    hasSymbol(e, symbol);
}

inline vector<PExpression> getEachSubexpression(const PExpression e) {
    vector<PExpression> subs;
    eachSubexpression(e, [&](PExpression& se) {
        assert(se != e);
        subs.push_back(se);
    });
    return subs;
}



// The caller is responsible for freeing the output.
// Input expressions are used by-pointer.
inline PExpression makeExpression(string symbol, vector<PExpression> subs = vector<PExpression>()) {

    auto e = allocate(symbol.length(), subs.size());
    assert(e->symbol_len == symbol.length());
    assert(e->subexpression_count == subs.size());

    memcpy(e->symbol, symbol.c_str(), e->symbol_len);

    eachSubexpressionInitIndexed(e, [&](PExpression& se, const int i) {
        se = subs[i];
        validate(se);
    });
    validateExpression(e, symbol);
    return e;
}

inline string getSymbol(PExpression e) {
    // TODO will not work with non-printable characters or other out-of-range bytes
    validate(e);
    auto s = string(e->symbol, e->symbol_len);
    validateExpression(e, s);
    return s;
}

// The caller is responsible for freeing the input and output
inline PExpression deepCopy(const PExpression e) {
    validate(e);
    auto o = allocate(e->symbol_len, e->subexpression_count);
    memcpy(o->symbol, e->symbol, e->symbol_len);

    eachSubexpressionInitIndexed(o, [&](PExpression& se, const int i) {
        se = deepCopy(e->subexpressions[i]);
    });

    assert(o != e);
    return o;
}

// The caller is responsible for freeing the detached expressions
inline vector<PExpression> getAndDetachEachSubexpression(const PExpression e) {
    vector<PExpression> subs;
    eachSubexpressionInit(e, [&](PExpression& se) {
        assert(e != se);
        validate(se);
        subs.push_back(se);

        se = 0;
    }, true);
    assert(subs.size() == e->subexpression_count);
    e->forget_subexpressions();
    assert(0 == e->subexpression_count);
    validate(e);
    return subs;
}

// The then-emptied expression is freed
inline vector<PExpression> getAndDetachEachSubexpressionAndDeallocate(PExpression& e) {
    auto val = getAndDetachEachSubexpression(e);
    deallocate(e);
    e = 0;
    return val;
}

// The caller is responsible for freeing the detached expressions
inline void detachAllSubexpressions(PExpression e) {
    getAndDetachEachSubexpression(e);
}

inline void filePersist(PExpression e, const char* filename) {
    validate(e);
    auto hFile = CreateFile(filename, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
    assert(hFile != INVALID_HANDLE_VALUE);
    persist(hFile, e);
    CloseHandle(hFile);
}

// returns 0 when failed
inline PExpression fileLoad(const char* filename) {
    PExpression e = 0;
    auto hFile = CreateFile(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) return 0;
    e = load(hFile);
    CloseHandle(hFile);
    return e;
}
inline bool isEmpty(PExpression x) {
    return x->symbol_len == 0
    && x->subexpression_count == 0
    && x->symbol == 0
  //  && x->subexpressions == 0
  ;
}

inline PExpression makeEmpty() {
    auto x = makeExpression("");
    assert(isEmpty(x));
    return x;
}
