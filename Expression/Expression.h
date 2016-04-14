#pragma once
#include <Windows.h>
// API
#ifdef EXPRESSION_DLL_COMPILE
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#pragma comment(lib, "Expression.lib")
#endif

/**
Memory management:
Any expression allocated via allocate must be (indirectly)
deallocated with deallocate.

allocate allocates space for pointers but does not initialize them to anything but 0x0.

All expressions in existence at any point in time must form a directed acyclic graph. It is an error for subexpressions to refer to the same expression.
*/
/*
In case you don't want to use the dll's .lib
struct ExpressionDll {

}
void dynamicLoad() {
HMODULE h = LoadLibraryA("Expression.dll");
assert(h != NULL);
assert(GetProcAddress(h, "persist"));
}
*/
struct Expression;
typedef Expression* PExpression;
extern "C" {
    API void persist(HANDLE hFile, PExpression);
    API PExpression load(HANDLE hFile);
    API PExpression allocate(unsigned int symbol_len, unsigned int subexpression_count);

    // Recursive i.e. deep delete
    API void deallocate(PExpression);

    API unsigned int countAllocated();
    API void validate(PExpression);
};

#include <vector>

#include "assert.h"
static const unsigned int INVALID_LENGTH = -1;

// ABI
// Lengths and counts are 0 iff the corresponding pointers are 0.
struct Expression {
private:
    unsigned int _symbol_len;
    char* _symbol;

    //unsigned int _subexpression_count;
    //Expression** _subexpressions;
    // easier debuggability: TODO is this good enough? why malloc if we dont have to
    // but it gives an STL dependency, and possible non-binary compat
    std::vector<Expression*> _subexpressions;

    // Control who gets to modify the attributes
    // Note that you may modify the stuff pointed to, just not the memory used.
    friend PExpression allocate(unsigned int symbol_len, unsigned int subexpression_count);
    friend void deallocate(PExpression);
    friend void validate(PExpression);
public:
#define readOnly(prop) decltype(_##prop) get_##prop() {return _##prop;}\
    __declspec(property(get = get_##prop)) decltype(_##prop) prop;

    readOnly(symbol_len);
    readOnly(symbol);
    
    unsigned int get_subexpression_count() {
        return _symbol_len == INVALID_LENGTH ? INVALID_LENGTH : _subexpressions.size();
    }
    __declspec(property(get = get_subexpression_count)) unsigned int subexpression_count;
    //Expression** // <- returning vector gives proper "subscript out of range" and is syntactically compatible
    std::vector<Expression*>& get_subexpressions() {
        assert(_symbol_len != INVALID_LENGTH);
        return //get_subexpression_count() == 0 ? 0 : 
            _subexpressions; // .data();
    }
    __declspec(property(get = get_subexpressions)) std::vector<Expression*>& subexpressions;
    
    // Warning know what you're doing
    void forget_subexpressions() {
        //_subexpression_count = 0;
        //_subexpressions = 0;
        _subexpressions.clear();
    }
};


