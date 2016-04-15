#include "Ruled.h"
#include "assert.h"
#include <Windows.h>
#include <sstream>
#include <Persist.h>
#include "ExpressionExt.h"
#include "filepersist.h"

SPExpression rules = 0;
const char* const rulesSymbol = "rules";
const char* const ruleSymbol = "rule";
const char* const callSymbol = "_call";
const char* const argSymbol = "arg";
const char* const rulesFilename = rulesSymbol;
// returns INT_MIN on failure
int strtol(std::string s) {
    stringstream ss; ss << s;
    long x;
    ss >> x;
    if (ss.fail()) return INT_MIN;
    return x;
}

int countRules() {
    return rules->subexpressions.size();
}

void validateRule(const SPExpression& rule) {
    assert(hasSymbol(rule, ruleSymbol));
    assert(rule->subexpressions[0]->symbol.length() > 0, "symbol of rule must have nonzero length");
    assert(rule->subexpressions[0]->subexpressions.size() == 0, "symbol of rule may not have children");
}
void validateRules() {
    assert(countRules() >= 0);
    for (const auto& rule : rules->subexpressions) {
        validateRule(rule);
    }
}

void store();
void addNewRule(vector<SPExpression> symbolAndExpr) {
    
    auto oldCount = countRules();
    auto newRule = makeExpression(ruleSymbol, std::move(symbolAndExpr));
    assert(symbolAndExpr.empty());
    auto ruleset = getAndDetachEachSubexpressionAndDeallocate(std::move(rules));

    ruleset.push_back(std::move(newRule));
    rules = makeExpression(rulesSymbol, std::move(ruleset));
    assert(ruleset.empty());

    assert(countRules() == oldCount + 1);
    
    store();
}

void init() {
    assert(!rules);
    if (!(rules = fileDepersist(rulesFilename))) {
        rules = makeExpression(rulesSymbol);
        assert(!countRules());
    }
    validateRules();
}

void store() {
    validateRules();
    filePersist(rules, rulesFilename);
}

void cleanup() {
    store();
    // somewhere after this, rules will be destroyed
}

bool isRuleName(const SPExpression& e) {
    //Tassert(e->symbol.length() > 0, "symbol of rule must have nonzero length");
    //Tassert(e->subexpressions.size() == 0, "symbol of rule may not have children");
    return e->symbol.length() > 0 && e->subexpressions.size() == 0;
}
// rulename()
DEF_TRANSFORMATION(deleteRule) {
    Tassert(isRuleName(e));

    auto f = makeExpression(rulesSymbol);
    for (auto& rule : rules->subexpressions) {
        if (rule->subexpressions[0]->symbol != e->symbol)
            f->subexpressions.push_back(std::move(rule));
    }
    rules = std::move(f);
    store();
    return makeEmpty();
}

DEF_TRANSFORMATION(getRule) {
    Tassert(isRuleName(e), "%s", e->symbol.c_str());

    for (auto& rule : rules->subexpressions) {
        if (rule->subexpressions[0]->symbol == e->symbol)
            return deepCopy(rule);
    }

    return makeEmpty();
}

DEF_TRANSFORMATION(addRule) {
    cout << "adding rule..." << endl;
    INPUT_IS("addRule", 2);
    Tassert(e->subexpressions.size() == 2);
    Tassert(isRuleName(e->subexpressions[0]), "%s", e->subexpressions[0]->symbol.c_str());

    deleteRule(deepCopy(e->subexpressions[0]));

    addNewRule(
        getAndDetachEachSubexpressionAndDeallocate(std::move(e))
        );

    return makeEmpty();
}

DEF_TRANSFORMATION(getRules) {
    Tassert(isEmpty(e));
    return deepCopy(rules);
}


bool endswith(string str, string end) {
    if (end.size() > str.size()) return false;
    return std::equal(end.rbegin(), end.rend(), str.rbegin());
}

// Returns the updated expression e, deleting as necessary
// argi is copied when used
// TODO this can be done with an if and a for each..
// The caller deallocates args and the return value (either returns e unchanged or deletes it while creating something new)
SPExpression deepReplaceArg(SPExpression e, const vector<SPExpression>& args) {
    if (hasSymbol(e, argSymbol)) {
        Tassert(e->subexpressions.size() == 1, "arg should be used with one subexpression");
        long j = strtol(e->subexpressions[0]->symbol); // TODO detect invalid
        Tassert(j < args.size() && j >= 0, "invalid number %d in arg, parsed from %s, args.size() = %d", j, e->subexpressions[0]->symbol.c_str(), args.size());
        return deepCopy(args[j]);
    }

    // Replace subexpressions with results from deepReplaceArg
    for (auto& se : e->subexpressions) {
        assert(e != se);
        se = deepReplaceArg(std::move(se), args);
    }
    return e;
}

DEF_TRANSFORMATION(_call) {
    INPUT_IS(callSymbol, 3);
    auto dllfilename_TransformationName_arg = getAndDetachEachSubexpressionAndDeallocate(std::move(e));
    //Tassert(e->subexpressions.size() == 0);// nah, e was moved!

    // dll
    auto dllfilename = std::move(dllfilename_TransformationName_arg[0]);
    Tassert(dllfilename->subexpressions.size() == 0);
    Tassert(dllfilename->symbol.length() > 0);
    Tassert(endswith(dllfilename->symbol, ".dll"), "dll filename must end in .dll");

    HMODULE m = LoadLibraryA(dllfilename->symbol.c_str());
    Tassert(m, "failed to load Library");

    // Transformation
    auto TransformationName = std::move(dllfilename_TransformationName_arg[1]);
    Tassert(TransformationName->subexpressions.size() == 0);
    Tassert(TransformationName->symbol.length() > 0);

    auto transformation = (TTransformation)GetProcAddress(m, TransformationName->symbol.c_str());
    Tassert(transformation, "could not find function %s", TransformationName->symbol.c_str());
    // TODO release m?

    // Arg
    e = transformation(std::move(dllfilename_TransformationName_arg[2]));
    assert(e);
    return e;
}

DEF_TRANSFORMATION(apply2) {
    return apply(apply(std::move(e)));
}

DEF_TRANSFORMATION(apply) {
    //for (bool changed = true; changed; ) {
        //changed = false;


        again:

        // Search rule
        for (auto& rule : rules->subexpressions) {
            assert(hasSymbol(rule, ruleSymbol));
            // rule(a b)
            // a?
            assert(rule->subexpressions.size() == 2);
            auto rule_lhs = rule->subexpressions[0]->symbol;
            assert(rule->subexpressions[0]->subexpressions.size() == 0);
            assert(rule_lhs.length() > 0);
            if (!hasSymbol(e, rule_lhs)) continue;

            const int exprSub = e->subexpressions.size();

            // Do replacement: Insert arg(0), arg(1), etc.
            auto arg0 = deepCopy(e);
            vector<SPExpression> args = getAndDetachEachSubexpressionAndDeallocate(std::move(e)); // arg(1 : n)
            args.insert(args.begin(), std::move(arg0)); // arg(0) == the whole expression
            
            // b
            e = deepCopy(rule->subexpressions[1]);
            assert(e != rule->subexpressions[1]);
            assert(args[0]->subexpressions.size() == exprSub);

            e = deepReplaceArg(std::move(e), args);
            assert(e);
            assert(e != rule->subexpressions[1]);

           // changed = true;
            break;
        }

        // is this a _call?
        if (hasSymbol(e, callSymbol)) {
            e = _call(std::move(e));
            assert(e);
        }

        // returned apply?
        if (hasSymbol(e, "apply")) {
            INPUT_IS("apply", 1);
            e = apply(std::move(e->subexpressions[0]));
            assert(e);
            //goto again;
        }
        assert(e);
    return e;
}

BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
    if (reason_for_call == DLL_PROCESS_ATTACH) init();
    else if(reason_for_call == DLL_PROCESS_DETACH) cleanup();
    return TRUE;
}