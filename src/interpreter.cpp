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
    auto maybeResult = interpreter.evaluate(*maybeAst);
    if (!maybeResult)
        return false;

    std::cout << print(*maybeResult) << std::endl;
    return true;
}


std::string print(const RuntimeValue& val)
{
    return std::visit([](auto&& arg) {
        return fmt::format("{}", arg);
    }, val);
}

std::optional<RuntimeValue> Interpreter::evaluate(ExpressionIndex expr)
{
    try
    {
        return eval(expr);
    }
    catch(const RuntimeError& e)
    {
        error(e.where.line, e.message);
        return std::nullopt;
    }
}

bool Interpreter::isTruthy(const RuntimeValue& val)
{
    if (auto boolVal = std::get_if<bool>(&val))
        return *boolVal;

    if (std::get_if<Nil>(&val))
        return false;
    
    return true;
}

void Interpreter::checkNumberOperand(const RuntimeValue& val, const Token& token)
{
    if (!std::get_if<double>(&val))
        throw RuntimeError{token, "Operand must evaluate to a number."};
}

RuntimeValue Interpreter::eval(ExpressionIndex expr)
{
    auto node = ctxt.getNode(expr);
    return std::visit(visitor, node);
}

RuntimeValue Interpreter::EvalVisitor::operator()(const Literal* l) const
{
    return std::visit([](auto&& arg) -> RuntimeValue {
        return arg;
    }, l->value.value);
}

RuntimeValue Interpreter::EvalVisitor::operator()(const Unary* u) const
{
    RuntimeValue inner = i.eval(u->subExpr);
    
    switch (u->op.type)
    {
    case TokenType::MINUS:
        checkNumberOperand(inner, u->op);
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
    RuntimeValue left = i.eval(b->left);
    RuntimeValue right = i.eval(b->right);

    switch (b->op.type)
    {
        // Arithmetic.
        case TokenType::SLASH:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) / std::get<double>(right);
        case TokenType::STAR:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) * std::get<double>(right);
        case TokenType::MINUS:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) - std::get<double>(right);
        case TokenType::PLUS:
            if (left.index() != right.index())
                throw RuntimeError{b->op, "Operands' type mismatch."};

            if (std::get_if<double>(&left))
                return std::get<double>(left) + std::get<double>(right);
            else if (std::get_if<std::string>(&left))
                return std::get<std::string>(left) + std::get<std::string>(right);

            throw RuntimeError{b->op, "Operands with unsupported type."};

        // Comparison.
        case TokenType::GREATER:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) > std::get<double>(right);
        case TokenType::GREATER_EQUAL:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) >= std::get<double>(right);
        case TokenType::LESS:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) < std::get<double>(right);
        case TokenType::LESS_EQUAL:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) <= std::get<double>(right);
        case TokenType::EQUAL_EQUAL:
            return left == right;

        default:
            break;
    }

    throw RuntimeError{b->op, "Unexpected value."};
}

RuntimeValue Interpreter::EvalVisitor::operator()(const Grouping* g) const
{
    return i.eval(g->subExpr);
}

