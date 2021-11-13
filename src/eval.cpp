#include <include/eval.h>

#include <fmt/format.h>
#include <chrono>

#include <include/utils.h>

using enum TokenType;

std::string print(const RuntimeValue& val)
{
    return std::visit([](auto&& arg) {
        return fmt::format("{}", arg);
    }, val);
}

RuntimeValue Callable::operator()(Interpreter& interp, std::vector<RuntimeValue> args)
{
    return impl(interp, args);
}

Interpreter::Interpreter(const ASTContext& ctxt, Environment env)
    : ctxt{ctxt}, globalEnv(std::move(env)), currentEnv(&globalEnv)
{
    // Built in functions.
    globalEnv.define("clock", Callable{0, [](Interpreter&, std::vector<RuntimeValue>) -> RuntimeValue
    {
        return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
    }});
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
    const auto& token = i.ctxt.getToken(l->value);

    switch(token.type)
    {
        case TRUE:
            return true;
        case FALSE:
            return false;
        default:
            break;
    }

    return std::visit([](auto&& arg) -> RuntimeValue {
        return arg;
    }, token.value);
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const Unary* u) const
{
    RuntimeValue inner = i.eval(u->subExpr);
    
    switch (i.ctxt.getToken(u->op).type)
    {
    case MINUS:
        checkNumberOperand(inner, u->op);
        return -std::get<double>(inner);

    case BANG:
        return !isTruthy(inner);
    
    default:
        break;
    }

    throw RuntimeError{u->op, "Unexpected unary operator."};
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const Binary* b) const
{
    RuntimeValue left = i.eval(b->left);

    // Short circut for logical operators.
    auto type = i.ctxt.getToken(b->op).type;
    if (type == OR)
    {
        if (isTruthy(left))
            return left;
        return i.eval(b->right);
    }
    else if (type == AND)
    {
        if (!isTruthy(left))
            return left;
        return i.eval(b->right);
    }

    RuntimeValue right = i.eval(b->right);

    switch (type)
    {
        // Arithmetic.
        case SLASH:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) / std::get<double>(right);
        case STAR:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) * std::get<double>(right);
        case MINUS:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) - std::get<double>(right);
        case PLUS:
            if (left.index() != right.index())
                throw RuntimeError{b->op, "Operands' type mismatch."};

            if (std::get_if<double>(&left))
                return std::get<double>(left) + std::get<double>(right);
            else if (std::get_if<std::string>(&left))
                return std::get<std::string>(left) + std::get<std::string>(right);

            throw RuntimeError{b->op, "Operands with unsupported type."};

        // Comparison.
        case GREATER:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) > std::get<double>(right);
        case GREATER_EQUAL:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) >= std::get<double>(right);
        case LESS:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) < std::get<double>(right);
        case LESS_EQUAL:
            checkNumberOperand(left, b->op);
            checkNumberOperand(right, b->op);
            return std::get<double>(left) <= std::get<double>(right);
        case EQUAL_EQUAL:
            return left == right;

        default:
            break;
    }

    throw RuntimeError{b->op, "Unexpected value."};
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const Assign* a) const
{
    RuntimeValue value = i.eval(a->value);
    if (!i.currentEnv->assign(std::get<std::string>(i.ctxt.getToken(a->name).value), value))
        throw RuntimeError{a->name, "Undefined variable."};

    return value;
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const Grouping* g) const
{
    return i.eval(g->subExpr);
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const DeclRef* r) const
{
    if (auto val = i.currentEnv->get(std::get<std::string>(i.ctxt.getToken(r->name).value)))
        return *val;

    throw RuntimeError{r->name, "Undefined variable."};
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const Call* c) const
{
    RuntimeValue callee = i.eval(c->callee);

    if (Callable* callable = get_if<Callable>(&callee))
    {
        if (callable->arity != c->args.size())
            throw RuntimeError{c->open,
                fmt::format("Expected {} arguments but got {}.", callable->arity, c->args.size())};

        std::vector<RuntimeValue> argValues;
        for(auto arg : c->args)
        {
            argValues.push_back(i.eval(arg));
        }

        return (*callable)(i, std::move(argValues));
    }

    throw RuntimeError{c->open, "Can only call functions and classes."};
}

void Interpreter::StmtEvalVisitor::operator()(const PrintStatement* s) const
{
    RuntimeValue value = i.eval(s->subExpr);
    fmt::print("{}\n", print(value));
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

    i.currentEnv->define(std::get<std::string>(i.ctxt.getToken(s->name).value), val);
}

void Interpreter::StmtEvalVisitor::operator()(const FunDecl* s) const
{
    Callable callable{
        static_cast<unsigned>(s->params.size()),
        [&params = s->params, body = s->body](Interpreter& interp,
                                              std::vector<RuntimeValue> args) -> RuntimeValue
        {
            // TODO: RAII
            auto previous = interp.currentEnv;
            Environment newEnv(interp.currentEnv);
            interp.currentEnv = &newEnv;

            // Bind arguments.
            for(unsigned i = 0; i < params.size(); ++i)
            {
                newEnv.define(std::get<std::string>(interp.ctxt.getToken(params[i]).value), args[i]);
            }

            interp.eval(body);

            interp.currentEnv = previous;

            // TODO: return values.
            return Nil{};
        }
    };

    i.currentEnv->define(std::get<std::string>(i.ctxt.getToken(s->name).value), callable);
}

void Interpreter::StmtEvalVisitor::operator()(const Block* s) const
{
    // TODO: RAII
    auto previous = i.currentEnv;
    Environment newEnv(i.currentEnv);
    i.currentEnv = &newEnv;

    for (auto child : s->statements)
    {
        i.eval(child);
    }

    i.currentEnv = previous;
}

void Interpreter::StmtEvalVisitor::operator()(const IfStatement* s) const
{
    if (isTruthy(i.eval(s->condition)))
        i.eval(s->thenBranch);
    else if (s->elseBranch)
        i.eval(*s->elseBranch);
}

void Interpreter::StmtEvalVisitor::operator()(const WhileStatement* s) const
{
    while (isTruthy(i.eval(s->condition)))
        i.eval(s->body);
}