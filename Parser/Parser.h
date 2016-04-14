#pragma once
#include "Expression.h"

#ifdef PARSE_DLL_COMPILE
#define API __declspec(dllexport)
#else
//#error Should be used via dynamic inclusion (Ruled.dll)
#define API __declspec(dllimport)
#pragma comment(lib, "Parser.lib")
#endif

extern "C" {
    API PExpression parse(PExpression symbol);
    API PExpression stringify(PExpression expr);
};
