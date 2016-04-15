#include <stdio.h>
#include <Windows.h>

#pragma warning(disable : 4002) // too many macro parameters is wrong
#pragma warning(disable : 4003) // assert does not need "commentFormat" and its arguments
//#pragma warning(error : 4005) // macro redefinition (e.g. of assert) is wrong
#undef assert


#define assert(x,commentFormat,...) {if(!(x)) {char _s[1000 /*not too much or heap*/]; sprintf_s(_s, "%s(%i) : Assertion failed : %s.\n\t<" commentFormat ">\n", __FILE__, __LINE__, #x, __VA_ARGS__); puts(_s); OutputDebugStringA(_s); DebugBreak(); OutputDebugStringA("! program continues after failed assertion\n\n"); } }

