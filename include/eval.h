#ifndef EVAL_H
#define EVAL_H

#include <variant>
#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include <fmt/format.h>

#include <include/ast.h>
#include <include/utils.h>

class Interpreter;
class Environment;

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
    Environment* closure;
    std::function<RuntimeValue(Interpreter&, std::vector<RuntimeValue>&&)> impl;

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

using Resolution = std::unordered_map<ExpressionIndex, int>;

// TODO: overhaul environment so each variable has a unique index
//       instead of relying on names.
class Environment
{
public:
    explicit Environment(Environment* enclosing = nullptr) noexcept : enclosing(enclosing) {}

    void define(const std::string& name, const RuntimeValue& value) noexcept
    {
        values.insert_or_assign(name, value);
    }

    bool assign(const std::string& name, const RuntimeValue& value) noexcept
    {
        if (auto it = values.find(name); it != values.end())
        {
            it->second = value;
            return true;
        }

        return false;
    }

    bool assignAt(int distance, const std::string& name, const RuntimeValue& value) noexcept
    {
        return ancestor(distance)->assign(name, value);
    }

    std::optional<RuntimeValue> get(const std::string& name) const noexcept
    {
        if (auto it = values.find(name); it != values.end())
            return it->second;

        return std::nullopt; 
    }

    std::optional<RuntimeValue> getAt(int distance, const std::string& name) const noexcept
    {
        return ancestor(distance)->get(name);
    }

private:
    Environment* ancestor(int distance) const noexcept
    {
        auto* result = const_cast<Environment*>(this);
        while(distance-- > 0)
            result = result->enclosing;
        
        return result;
    }

    std::unordered_map<std::string, RuntimeValue> values;

    Environment* enclosing;
    friend Interpreter;
};

// Evaluation logic.
class Interpreter
{
public:
    Interpreter(const ASTContext& ctxt, const DiagnosticEmitter& diag, Environment env = Environment{});

    bool evaluate(StatementIndex stmt);

    const ASTContext& getContext() const noexcept { return ctxt; }
    Environment& getGlobalEnv() noexcept { return globalEnv; }
    Environment& getCurrentEnv() noexcept { return stack.empty() ? getGlobalEnv() : *stack.back(); }
private:
    RuntimeValue eval(ExpressionIndex expr);
    void eval(StatementIndex stmt);

    static bool isTruthy(const RuntimeValue& val);
    static void checkNumberOperand(const RuntimeValue& val, Index<Token> token);
    Environment* pushEnv(Environment* current);
    void popEnv();
    void collect();

    const ASTContext& ctxt;
    const DiagnosticEmitter& diag;

    Environment globalEnv;
    std::vector<Environment*> stack;
    Resolution resolution;

    std::unordered_set<std::unique_ptr<Environment>> allEnvs;
    unsigned collectCounter;

    ExpressionIndex currentExpr; // TODO: get rid of this.

    struct ExprEvalVisitor
    {
        Interpreter& i;
        RuntimeValue operator()(const Binary* b) const;
        RuntimeValue operator()(const Assign* a) const;
        RuntimeValue operator()(const Unary* u) const;
        RuntimeValue operator()(const Literal* l) const;
        RuntimeValue operator()(const Grouping* g) const;
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
        void operator()(const Unit* s) const;
    } stmtVisitor{*this};
};

#endif