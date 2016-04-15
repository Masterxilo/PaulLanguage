#include "Expression.h"
#include "assert.h"
#include <Windows.h>

SPExpression allocate() {
    auto e = new Expression;
    return scope(e);
}

void deallocate(PExpression e) {
    assert(e);
    /*for (auto& s : e->subexpressions) {
        deallocate(s.release());
        }*/
    delete e; // should delete all children for which we have SPExpression unique_ptr
}