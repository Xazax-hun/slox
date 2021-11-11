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

// TODO: use a proper REPL library like readline.
bool runPrompt(bool dumpAst)
{
    std::string line;
    // TODO: more sophisticated solution for persisting env across lines.
    Environment env;
    Parser parser;
    while (true)
    {
        std::cout << ::prompt;
        if (!std::getline(std::cin, line))
            break;
        Lexer lexer(line);
        auto maybeTokens = lexer.lexAll();
        if (!maybeTokens)
            return false;

        auto maybeAst = parser.parse(std::move(*maybeTokens));
        if (!maybeAst)
            return false;

        if (dumpAst)
        {
            ASTPrinter printer(parser.getContext());
            std::cout << printer.print(*maybeAst) << std::endl;
        }

        Interpreter interpreter(parser.getContext(), env);
        if (!interpreter.evaluate(*maybeAst))
            return false;
        
        env = interpreter.getEnv();
    }
    return true;
}

bool runSource(std::string sourceText, bool dumpAst)
{
    Lexer lexer(sourceText);
    auto maybeTokens = lexer.lexAll();
    if (!maybeTokens)
        return false;

    Parser parser;
    auto maybeAst = parser.parse(std::move(*maybeTokens));
    if (!maybeAst)
        return false;

    if (dumpAst)
    {
        ASTPrinter printer(parser.getContext());
        std::cout << printer.print(*maybeAst) << std::endl;
    }

    Interpreter interpreter(parser.getContext());
    if (!interpreter.evaluate(*maybeAst))
        return false;

    return true;
}


std::string print(const RuntimeValue& val)
{
    return std::visit([](auto&& arg) {
        return fmt::format("{}", arg);
    }, val);
}

bool Interpreter::evaluate(StatementIndex stmt)
{
    try
    {
        eval(stmt);
        return true;
    }
    catch(const RuntimeError& e)
    {
        error(ctxt.getToken(e.where).line, e.message);
        return false;
    }
}

std::optional<RuntimeValue> Interpreter::evaluate(ExpressionIndex expr)
{
    try
    {
        return eval(expr);
    }
    catch(const RuntimeError& e)
    {
        error(ctxt.getToken(e.where).line, e.message);
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

void Interpreter::checkNumberOperand(const RuntimeValue& val, Index<Token> token)
{
    if (!std::get_if<double>(&val))
        throw RuntimeError{token, "Operand must evaluate to a number."};
}

RuntimeValue Interpreter::eval(ExpressionIndex expr)
{
    auto node = ctxt.getNode(expr);
    return std::visit(exprVisitor, node);
}

void Interpreter::eval(StatementIndex stmt)
{
    auto node = ctxt.getNode(stmt);
    std::visit(stmtVisitor, node);
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const Literal* l) const
{
    return std::visit([](auto&& arg) -> RuntimeValue {
        return arg;
    }, i.ctxt.getToken(l->value).value);
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const Unary* u) const
{
    RuntimeValue inner = i.eval(u->subExpr);
    
    switch (i.ctxt.getToken(u->op).type)
    {
    case TokenType::MINUS:
        checkNumberOperand(inner, u->op);
        return -std::get<double>(inner);

    case TokenType::BANG:
        return !isTruthy(inner);
    
    default:
        break;
    }

    throw RuntimeError{u->op, "Unexpected unary operator."};
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const Binary* b) const
{
    RuntimeValue left = i.eval(b->left);
    RuntimeValue right = i.eval(b->right);

    switch (i.ctxt.getToken(b->op).type)
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

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const Assign* a) const
{
    RuntimeValue value = i.eval(a->value);
    if (!i.globalEnv.assign(std::get<std::string>(i.ctxt.getToken(a->name).value), value))
        throw RuntimeError{a->name, "Undefined variable."};

    return value;
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const Grouping* g) const
{
    return i.eval(g->subExpr);
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const DeclRef* r) const
{
    if (auto val = i.globalEnv.get(std::get<std::string>(i.ctxt.getToken(r->name).value)))
        return *val;

    throw RuntimeError{r->name, "Undefined variable."};
}

void Interpreter::StmtEvalVisitor::operator()(const PrintStatement* s) const
{
    RuntimeValue value = i.eval(s->subExpr);
    std::cout << print(value) << "\n";
}

void Interpreter::StmtEvalVisitor::operator()(const ExprStatement* s) const
{
    i.eval(s->subExpr);
}

void Interpreter::StmtEvalVisitor::operator()(const VarDecl* s) const
{
    RuntimeValue val;
    if (s->init)
        val = i.eval(*s->init);
    else
        val = Nil{};

    i.globalEnv.define(std::get<std::string>(i.ctxt.getToken(s->name).value), val);
}

