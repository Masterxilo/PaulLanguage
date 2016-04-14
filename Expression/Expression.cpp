#include "Expression.h"
#include "assert.h"

void checkAllocOr0(void* p, unsigned int len) {
    assert(len != INVALID_LENGTH);
    assert((p && len != 0) || (!p && len == 0));
}

template<typename T>
void allocOr0(T*& p, int count) {
    if (!count) p = 0;
    else {
        const auto sz = sizeof(T) * count;
        p = (T*)malloc(sz);
        memset(p, 0x0, sz);
    }

    checkAllocOr0(p, count);
}

static HANDLE hFile = 0;
static void Write(void* what, unsigned int len) {
    assert(hFile != INVALID_HANDLE_VALUE, "invalid handle");
    DWORD written;
    WriteFile(hFile, what, len, &written, 0);
    assert(written == len, "%d %d", written, len);
}

static void Read(void* to, unsigned int len) {
    assert(hFile != INVALID_HANDLE_VALUE, "invalid handle");
    DWORD read;
    ReadFile(hFile, to, len, &read, 0);
    assert(read == len, "%d %d", read, len);
}

static void WriteInt(unsigned int val) {
    Write(&val, sizeof(val));
}

static unsigned int ReadInt() {
    unsigned int val;
    Read(&val, sizeof(val));
    return val;
}

extern "C" {
    API void persist(HANDLE hFile, PExpression e) {
        ::hFile = hFile;
        assert(e);

        WriteInt(e->symbol_len);
        WriteInt(e->subexpression_count);

        Write(e->symbol, e->symbol_len);
        
        for (int i = 0; i < e->subexpression_count; i++) {
            persist(hFile, e->subexpressions[i]);
        }
        validate(e);
    }

    API PExpression load(HANDLE hFile) {
        ::hFile = hFile;

        unsigned int symbol_len = ReadInt();
        unsigned int subexpression_count = ReadInt();

        PExpression e = allocate(symbol_len, subexpression_count);
        assert(e->symbol_len == symbol_len);
        assert(e->subexpression_count == subexpression_count);

        Read(e->symbol, e->symbol_len);

        for (int i = 0; i < e->subexpression_count; i++) {
            e->subexpressions[i] = load(hFile);
        }
        validate(e);
        return e;
    }

    static unsigned int allocatedCount = 0;
    API unsigned int countAllocated() {
        return allocatedCount;
    }

    // Does not check subexpressions
    static void validateFresh(PExpression e) {
        assert(e);
        assert(e->subexpression_count != INVALID_LENGTH);
        assert(e->symbol_len != INVALID_LENGTH);

        checkAllocOr0(e->symbol, e->symbol_len);
        //checkAllocOr0(e->subexpressions, e->subexpression_count);
    }

    API PExpression allocate(unsigned int symbol_len, unsigned int subexpression_count) {
        allocatedCount++;

        auto e = new Expression;//(PExpression)malloc(sizeof(Expression));
        assert(e);
        e->_symbol_len = symbol_len;
        allocOr0(e->_symbol, symbol_len);

        /*
        e->_subexpression_count = subexpression_count;
        allocOr0(e->_subexpressions, subexpression_count);
        for (int i = 0; i < e->subexpression_count; i++) {
            assert(!e->subexpressions[i]);
        }*/
        e->_subexpressions.resize(subexpression_count);

        validateFresh(e);
        return e;
    }


    API void validate(PExpression e) {
        validateFresh(e);

        // writeability
        e->_symbol_len = e->_symbol_len;
        if (e->symbol_len) e->_symbol[0] = e->_symbol[0];

        for (int i = 0; i < e->subexpression_count; i++) {
            assert(e->subexpressions[i]);
        }
    }

    API void deallocate(PExpression e) {
        validate(e);
        allocatedCount--;

        for (int i = 0; i < e->subexpression_count; i++) {
            deallocate(e->subexpressions[i]);
            //e->subexpressions[i] = 0; // for later fault detection
        }

        free(e->symbol);
        //free(e->subexpressions);

        // invalidate for later fault detection (e.g. double freeing)
        //e->_subexpression_count = INVALID_LENGTH;
        e->_symbol_len = INVALID_LENGTH;
        //free(e);
        delete e;
    }
};