#include "Ruled.h"
#include "assert.h"
#include <Windows.h>
#include <sstream>
#include "ExpressionExt.h"

/** TODO cache (loaded modules) and procaddrs <- maybe windows is faster, at least with the modules. Hasing for string pair?
#include <unordered_map>
using namespace std;
unordered_map < string, HMODULE > loadedLibraries;
*/

/**
Format of rules:
rules(
    rule(a b(...))

    rule(a b(...))
)

Where a is a single symbol and the expression b may use argument identifiers
arg(0), arg(1) etc.
*/
PExpression rules = 0;
const char* const rulesSymbol = "rules";
const char* const ruleSymbol = "rule";
const char* const callSymbol = "_call";
const char* const argSymbol = "arg";
const char* const rulesFilename = rulesSymbol;

int strtol(std::string s) {
    stringstream ss; ss << s;
    long x;
    ss >> x;
    return x;
}

void validateRules() {
    validateExpression(rules, rulesSymbol);
}

int countRules() {
    validateRules();
    return rules->subexpression_count;
}

void store();
void addNewRule(vector<PExpression> symbolAndExpr) {
    assert(symbolAndExpr.size() == 2);
    auto symbol = symbolAndExpr[0];
    assert(symbol->symbol_len > 0, "symbol of rule must have nonzero length");
    assert(symbol->subexpression_count == 0, "symbol of rule may not have children");

    auto oldCount = countRules();
    auto newRule = makeExpression(ruleSymbol, symbolAndExpr);
    auto ruleset = getAndDetachEachSubexpressionAndDeallocate(rules);

    ruleset.push_back(newRule);
    rules = makeExpression(rulesSymbol, ruleset);
    validateRules();

    assert(countRules() == oldCount + 1);

    store();
}

void init() {
    assert(!rules);
    if (!(rules = fileLoad(rulesFilename))) {
        rules = makeExpression(rulesSymbol);
        assert(rules->subexpression_count == 0);
    }
    validateRules();
}

void store() {
    validateRules();
    filePersist(rules, rulesFilename);
}

void cleanup() {
    store();
    deallocate(rules);
}

extern "C" {

    API PExpression addRule(PExpression addRule_symbol_expression) {
        validateExpression(addRule_symbol_expression, "addRule");
        assert(addRule_symbol_expression->subexpression_count == 2);
        addNewRule(
            getAndDetachEachSubexpressionAndDeallocate(addRule_symbol_expression)
            );

        return makeEmpty();
    }

    API PExpression getRules(PExpression e) {
        validateRules();
        assert(isEmpty(e));
        deallocate(e);
        return deepCopy(rules);
    }


    bool endswith(string str, string end) {
        if (end.size() > str.size()) return false;
        return std::equal(end.rbegin(), end.rend(), str.rbegin());
    }

    // Returns the updated expression e, deleting as necessary
    // argi is copied when used
    // TODO this can be done with an if and a for each..
    PExpression deepReplaceArg(PExpression e, const vector<PExpression>& args) {
        if (hasSymbol(e, argSymbol)) {
            assert(e->subexpression_count == 1);
            long j = strtol(getSymbol(e->subexpressions[0]));
            assert(j < args.size());
            
            deallocate(e);
            return deepCopy(args[j]);
        }

        // Replace subexpressions with results from deepReplaceArg
        eachSubexpression(e, [&](PExpression& se) {
            assert(e != se);
            se = deepReplaceArg(se, args);
        });
        return e;
    }

    API PExpression _call(PExpression call_dllfilename_TransformationName_arg) {
        // TODO for optimization, we might consider not copying things used only once
        // and we could tolerate no-argument (passing NULL or an empty expression)
        validateExpression(call_dllfilename_TransformationName_arg, callSymbol);
        assert(call_dllfilename_TransformationName_arg->subexpression_count == 3);
        auto dllfilename = call_dllfilename_TransformationName_arg->subexpressions[0];
        assert(dllfilename->subexpression_count == 0);
        assert(dllfilename->symbol_len > 0);
        assert(endswith(getSymbol(dllfilename), ".dll"));

        auto TransformationName = call_dllfilename_TransformationName_arg->subexpressions[1];
        assert(TransformationName->subexpression_count == 0);
        assert(TransformationName->symbol_len > 0);
        auto arg = call_dllfilename_TransformationName_arg->subexpressions[2];

        //
        detachAllSubexpressions(call_dllfilename_TransformationName_arg);
        deallocate(call_dllfilename_TransformationName_arg);
        
        auto lfn = getSymbol(dllfilename);
        HMODULE m = LoadLibraryA(lfn.c_str());
        deallocate(dllfilename);
        assert(m);

        auto tfn = getSymbol(TransformationName);
        deallocate(TransformationName);
        auto transformation = (TTransformation)GetProcAddress(m, tfn.c_str());
        assert(transformation);
        // TODO release m?

        return transformation(arg); // the transformation will deallocate the argument if necessary
    }

    API_IMPORT_ALWAYS PExpression apply(PExpression expr) {
        for (bool changed = true; changed; ) {
            changed = false;

            // is this an _call?
            if (hasSymbol(expr, callSymbol)) {
                expr = _call(expr);
                changed = true;
                continue;
            }

            // Search rule
            eachSubexpression(rules, [&](PExpression& rule) {
                assert(rule->subexpression_count == 2);
                auto rule_symbol = getSymbol(rule->subexpressions[0]);
                if (!hasSymbol(expr, rule_symbol)) return;

                const int exprSub = expr->subexpression_count;

                // Do replacement: Insert arg(0), arg(1), etc.
                vector<PExpression> args = getEachSubexpression(expr); // arg(1 : n)
                args.insert(args.begin(), deepCopy(expr)); // arg(0) == the whole expression

                for (auto & a : args) {
                    assert(a->symbol_len < 100 * 1000); // sanity check
                    validate(a);
                }

                detachAllSubexpressions(expr);
                deallocate(expr);

                for (auto & a : args) {
                    assert(a->symbol_len < 100 * 1000); // sanity check
                    validate(a);
                }

                expr = deepCopy(rule->subexpressions[1]);
                assert(expr != rule->subexpressions[1]);
                assert(args[0]->subexpression_count == exprSub);

                expr = deepReplaceArg(expr, args);
                assert(expr != rule->subexpressions[1]);

                changed = true;
            });

        }

        return expr;
    }
}

BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
    if (reason_for_call == DLL_PROCESS_ATTACH) init();
    else if(reason_for_call == DLL_PROCESS_DETACH) cleanup();
    return TRUE;
}