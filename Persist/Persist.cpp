#define PERSIST_DLL_COMPILE
#include "Persist.h"
#include "assert.h"

static void Write(HANDLE hFile, const void* what, unsigned int len) {
    assert(hFile != INVALID_HANDLE_VALUE, "invalid handle");
    DWORD written;
    assert(WriteFile(hFile, what, len, &written, 0));
    assert(written == len, "%d %d", written, len);
}

bool readOk = true;
static void Read(HANDLE hFile, void* to, unsigned int len) {
    assert(readOk);
    assert(hFile != INVALID_HANDLE_VALUE, "invalid handle");
    DWORD read;
    if (!ReadFile(hFile, to, len, &read, 0)) readOk = false;
    if (read != len) readOk = false;//assert(read == len, "%d %d", read, len);
}

// used externally
__declspec(dllexport) void Read(HANDLE hFile, std::string& to) {
    Read(hFile, (char*)to.c_str(), to.length());
}

__declspec(dllexport) void Write(HANDLE hFile, const std::string& what) {
    Write(hFile, what.c_str(), what.length());
}

static void WriteInt(HANDLE hFile, unsigned int val) {
    Write(hFile, &val, sizeof(val));
}

static unsigned int ReadInt(HANDLE hFile) {
    unsigned int val;
    Read(hFile, &val, sizeof(val));
    return val;
}

API void persist(HANDLE hFile, const SPExpression& e) {
    assert(e);

    WriteInt(hFile, e->symbol.length());
    WriteInt(hFile, e->subexpressions.size());

    Write(hFile, e->symbol);

    for (auto& s : e->subexpressions) {
        persist(hFile, s);
    }
}

#include "expressionext.h"
API SPExpression load(HANDLE hFile) {
    SPExpression e = allocate();
    ::readOk = true;

    auto i = ReadInt(hFile); if (!::readOk) return makeEmpty();
    e->symbol.resize(i);

    i = ReadInt(hFile); if (!::readOk) return makeEmpty();
    e->subexpressions.resize(i);

    Read(hFile, e->symbol); if (!::readOk) return makeEmpty();

    for (auto& s : e->subexpressions) {
        s = load(hFile);
    }
    return e;
}