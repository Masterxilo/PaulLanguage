#pragma once

/*++

Module Name:

    FilePersist.h

Abstract:

    Persist.h wrapper that creates a file on the file system.

--*/

#include "Expression.h"
#include <functional>
#include "assert.h"
#include "Persist.h"
#include <string>
#include <Windows.h>
#include <vector>
#include <memory>
#include <iostream>
using namespace std;


// The caller is responsible for freeing the argument.
inline void filePersist(const SPExpression& e, const char* filename) {
    auto hFile = CreateFile(filename, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
    assert(hFile != INVALID_HANDLE_VALUE);
    persist(hFile, e);
    CloseHandle(hFile);
}

// The caller is responsible for freeing the return value.
// returns 0 when failed
inline SPExpression fileDepersist(const char* filename) {
    auto hFile = CreateFile(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) return 0;
    SPExpression e = load(hFile);
    CloseHandle(hFile);
    return e;
}