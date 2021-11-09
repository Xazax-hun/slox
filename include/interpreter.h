#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string_view>
#include <string>
#include <variant>

#include <include/ast.h>
#include <fmt/format.h>

bool runFile(std::string_view path, bool dumpAst = false);
bool runSource(std::string sourceText, bool dumpAst = false);
bool runPrompt(bool dumpAst = false);

struct Nil{};
template <> struct fmt::formatter<Nil> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(Nil, FormatContext& ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "nil");
  }
};
inline bool operator==(Nil, Nil) { return true; }

// TODO: Add representation of objects.
using RuntimeValue = std::variant<Nil, std::string, double, bool>;

std::string print(const RuntimeValue&);

class Interpreter
{
public:
    Interpreter(const ASTContext& ctxt) : ctxt{ctxt}, visitor{*this} {}

    RuntimeValue evaluate(ExpressionIndex expr);

private:

    static bool isTruthy(const RuntimeValue& val);

    const ASTContext& ctxt;

    struct EvalVisitor
    {
        Interpreter& i;
        RuntimeValue operator()(const Binary* b) const;
        RuntimeValue operator()(const Unary* u) const;
        RuntimeValue operator()(const Literal* l) const;
        RuntimeValue operator()(const Grouping* l) const;
    } visitor;
};

#endif