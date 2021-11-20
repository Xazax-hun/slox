#ifndef EVAL_H
#define EVAL_H

#include <variant>
#include <vector>
#include <stack>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include <include/ast.h>
#include <fmt/format.h>

struct Interpreter;

// Representing runtime values.
struct Nil{};
template <>
struct fmt::formatter<Nil>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(Nil, FormatContext& ctx) -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "nil");
    }
};
inline bool operator==(Nil, Nil) { return true; }

struct Callable;

// TODO: Add representation of objects.
using RuntimeValue = std::variant<Nil, Callable, std::string, double, bool>;

struct RuntimeError
{
    Index<Token> where;
    std::string message;
};

// Using throw to return. (TODO: revise.)
struct Callable
{
    unsigned arity;
    std::function<RuntimeValue(Interpreter&, std::vector<RuntimeValue>)> impl;

    RuntimeValue operator()(Interpreter& interp, std::vector<RuntimeValue> args);
};

struct ReturnValue
{
    std::optional<RuntimeValue> value;
};

template <>
struct fmt::formatter<Callable>
{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const Callable&, FormatContext& ctx) -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "<Callable>");
    }
};
inline bool operator==(const Callable&, const Callable&) { return false; }

std::string print(const RuntimeValue&);

class Environment
{
public:
    Environment(Environment* enclosing = nullptr) : enclosing(enclosing) {}

    void define(const std::string& name, const RuntimeValue& value)
    {
        values.insert_or_assign(name, value);
    }

    bool assign(const std::string& name, const RuntimeValue& value)
    {
        if (auto it = values.find(name); it != values.end())
        {
            it->second = value;
            return true;
        }

        if (enclosing)
            return enclosing->assign(name, value);

        return false;
    }

    std::optional<RuntimeValue> get(const std::string& name) const
    {
        if (auto it = values.find(name); it != values.end())
            return it->second;

        if (enclosing)
            return enclosing->get(name);

        return std::nullopt; 
    }

private:
    // TODO: make this a string view.
    std::unordered_map<std::string, RuntimeValue> values;

    Environment* enclosing;
};

// Evaluation logic.
class Interpreter
{
public:
    Interpreter(const ASTContext& ctxt, Environment env = {});

    std::optional<RuntimeValue> evaluate(ExpressionIndex expr);
    bool evaluate(StatementIndex stmt);

    const ASTContext& getContext() { return ctxt; }
    Environment& getGlobalEnv() { return globalEnv; }
    Environment& getCurrentEnv() { return *stack.top(); }
private:
    RuntimeValue eval(ExpressionIndex expr);
    void eval(StatementIndex expr);

    static bool isTruthy(const RuntimeValue& val);
    static void checkNumberOperand(const RuntimeValue& val, Index<Token> token);
    void collect();
    Environment* pushEnv(Environment* current);
    void popEnv();

    const ASTContext& ctxt;
    Environment globalEnv;
    std::stack<Environment*> stack;
    std::unordered_set<std::unique_ptr<Environment>> allEnvs;

    struct ExprEvalVisitor
    {
        Interpreter& i;
        RuntimeValue operator()(const Binary* b) const;
        RuntimeValue operator()(const Assign* a) const;
        RuntimeValue operator()(const Unary* u) const;
        RuntimeValue operator()(const Literal* l) const;
        RuntimeValue operator()(const Grouping* l) const;
        RuntimeValue operator()(const DeclRef* r) const;
        RuntimeValue operator()(const Call* c) const;
    } exprVisitor{*this};

    struct StmtEvalVisitor
    {
        Interpreter& i;
        void operator()(const PrintStatement* s) const;
        void operator()(const ExprStatement* s) const;
        void operator()(const VarDecl* s) const;
        void operator()(const FunDecl* s) const;
        void operator()(const Return* s) const;
        void operator()(const Block* s) const;
        void operator()(const IfStatement* s) const;
        void operator()(const WhileStatement* s) const;
    } stmtVisitor{*this};
};

#endif