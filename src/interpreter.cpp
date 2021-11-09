#include <include/interpreter.h>

#include <fstream>
#include <sstream>
#include <iostream>

#include <include/lexer.h>
#include <include/ast.h>
#include <include/parser.h>

constexpr std::string_view prompt{"> "};

bool runFile(std::string_view path, bool dumpAst)
{
    std::ifstream file(path.data());
    if (!file) 
        return false;

    // TODO: Do we might not need to store
    //       the full source text in memory.
    std::stringstream buffer;
    buffer << file.rdbuf();
    return runSource(std::move(buffer).str(), dumpAst);
}

bool runPrompt(bool dumpAst)
{
    std::string line;
    while (true)
    {
        std::cout << ::prompt;
        if (!std::getline(std::cin, line))
            break;
        if (!runSource(std::move(line), dumpAst))
            return false;
    }
    return true;
}

bool runSource(std::string sourceText, bool dumpAst)
{
    Lexer lexer(sourceText);
    auto maybeTokens = lexer.lexAll();
    if (!maybeTokens)
        return false;

    Parser parser(std::move(*maybeTokens));
    auto maybeAst = parser.parse();
    if (!maybeAst)
        return false;

    if (dumpAst)
    {
        ASTPrinter printer(parser.getContext());
        std::cout << printer.print(*maybeAst) << std::endl;
    }

    Interpreter interpreter(parser.getContext());
    std::cout << print(interpreter.evaluate(*maybeAst)) << std::endl;

    return true;
}


std::string print(const RuntimeValue& val)
{
    return std::visit([](auto&& arg) {
        return fmt::format("{}", arg);
    }, val);
}

RuntimeValue Interpreter::evaluate(ExpressionIndex expr)
{
    auto node = ctxt.getNode(expr);
    return std::visit(visitor, node);
}

bool Interpreter::isTruthy(const RuntimeValue& val)
{
    if (auto boolVal = std::get_if<bool>(&val))
        return *boolVal;

    if (std::get_if<Nil>(&val))
        return false;
    
    return true;
}

RuntimeValue Interpreter::EvalVisitor::operator()(const Literal* l) const
{
    return std::visit([](auto&& arg) -> RuntimeValue {
        return arg;
    }, l->value.value);
}

RuntimeValue Interpreter::EvalVisitor::operator()(const Unary* u) const
{
    RuntimeValue inner = i.evaluate(u->subExpr);
    
    switch (u->op.type)
    {
    case TokenType::MINUS:
        // TODO: runtime check
        return -std::get<double>(inner);

    case TokenType::BANG:
        return !isTruthy(inner);
    
    default:
        break;
    }

    // TODO: report error.
    return Nil{};
}

RuntimeValue Interpreter::EvalVisitor::operator()(const Binary* b) const
{
    RuntimeValue left = i.evaluate(b->left);
    RuntimeValue right = i.evaluate(b->right);

    switch (b->op.type)
    {
        // Arithmetic.
        case TokenType::SLASH:
            // TODO: runtime check
            return std::get<double>(left) / std::get<double>(right);
        case TokenType::STAR:
            // TODO: runtime check
            return std::get<double>(left) * std::get<double>(right);
        case TokenType::MINUS:
            // TODO: runtime check
            return std::get<double>(left) - std::get<double>(right);
        case TokenType::PLUS:
            // TODO: runtime check and string concat
            return std::get<double>(left) + std::get<double>(right);

        // Comparison.
        case TokenType::GREATER:
            // TODO: runtime check
            return std::get<double>(left) > std::get<double>(right);
        case TokenType::GREATER_EQUAL:
            // TODO: runtime check
            return std::get<double>(left) >= std::get<double>(right);
        case TokenType::LESS:
            // TODO: runtime check
            return std::get<double>(left) < std::get<double>(right);
        case TokenType::LESS_EQUAL:
            // TODO: runtime check
            return std::get<double>(left) <= std::get<double>(right);
        case TokenType::EQUAL_EQUAL:
            return left == right;

        default:
            break;
    }

    // TODO: report error.
    return Nil{};
}

RuntimeValue Interpreter::EvalVisitor::operator()(const Grouping* g) const
{
    return i.evaluate(g->subExpr);
}

