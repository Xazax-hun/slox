#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string_view>
#include <string>
#include <variant>
#include <unordered_map>

#include <include/ast.h>
#include <fmt/format.h>

// Main entry points.
bool runFile(std::string_view path, bool dumpAst = false);
bool runSource(std::string sourceText, bool dumpAst = false);
bool runPrompt(bool dumpAst = false);

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

struct RuntimeError
{
    Index<Token> where;
    std::string message;
};

// TODO: Add representation of objects.
using RuntimeValue = std::variant<Nil, std::string, double, bool>;
std::string print(const RuntimeValue&);

class Environment
{
public:
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

        return false;
    }

    std::optional<RuntimeValue> get(const std::string& name) const
    {
        if (auto it = values.find(name); it != values.end())
            return it->second;

        return std::nullopt; 
    }

private:
    // TODO: make this a string view.
    std::unordered_map<std::string, RuntimeValue> values;
};

// Evaluation logic.
class Interpreter
{
public:
    Interpreter(const ASTContext& ctxt, Environment env = {})
        : ctxt{ctxt}, globalEnv(std::move(env)) {}

    std::optional<RuntimeValue> evaluate(ExpressionIndex expr);
    bool evaluate(StatementIndex stmt);

    const ASTContext& getContext() { return ctxt; }
    Environment getEnv() { return globalEnv; }
private:
    RuntimeValue eval(ExpressionIndex expr);
    void eval(StatementIndex expr);

    static bool isTruthy(const RuntimeValue& val);
    static void checkNumberOperand(const RuntimeValue& val, Index<Token> token);

    const ASTContext& ctxt;
    Environment globalEnv;

    struct ExprEvalVisitor
    {
        Interpreter& i;
        RuntimeValue operator()(const Binary* b) const;
        RuntimeValue operator()(const Assign* a) const;
        RuntimeValue operator()(const Unary* u) const;
        RuntimeValue operator()(const Literal* l) const;
        RuntimeValue operator()(const Grouping* l) const;
        RuntimeValue operator()(const DeclRef* r) const;
    } exprVisitor{*this};

    struct StmtEvalVisitor
    {
        Interpreter& i;
        void operator()(const PrintStatement* s) const;
        void operator()(const ExprStatement* s) const;
        void operator()(const VarDecl* s) const;
    } stmtVisitor{*this};
};

#endif