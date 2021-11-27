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
    : ctxt{ctxt}, globalEnv(std::move(env)), collectCounter(0)
{
    // Built in functions.
    globalEnv.define("clock",
        Callable{
            0, &globalEnv,
            [](Interpreter&, std::vector<RuntimeValue>) -> RuntimeValue
            {
                return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
            }
        }
    );
    stack.push_back(&globalEnv);
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
    const auto& varName = std::get<std::string>(i.ctxt.getToken(a->name).value);
    if (!i.getCurrentEnv().assign(varName, value))
        throw RuntimeError{a->name, fmt::format("Undefined variable: '{}'.", varName)};

    return value;
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const Grouping* g) const
{
    return i.eval(g->subExpr);
}

RuntimeValue Interpreter::ExprEvalVisitor::operator()(const DeclRef* r) const
{
    if (auto val = i.getCurrentEnv().get(std::get<std::string>(i.ctxt.getToken(r->name).value)))
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

        auto retVal = (*callable)(i, std::move(argValues));
        i.collect();
        return retVal;
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

    i.getCurrentEnv().define(std::get<std::string>(i.ctxt.getToken(s->name).value), val);
}

void Interpreter::StmtEvalVisitor::operator()(const FunDecl* s) const
{
    Callable callable{
        static_cast<unsigned>(s->params.size()),
        &i.getCurrentEnv(),
        [&params = s->params, body = s->body, closure = &i.getCurrentEnv()](
            Interpreter& interp,
            std::vector<RuntimeValue> args) -> RuntimeValue
        {
            auto *newEnv = interp.pushEnv(closure);

            // Bind arguments.
            for(unsigned i = 0; i < params.size(); ++i)
            {
                newEnv->define(std::get<std::string>(interp.ctxt.getToken(params[i]).value), args[i]);
            }

            try
            {
                for (auto stmt : body)
                {
                    interp.eval(stmt);
                }
            }
            catch (const ReturnValue& retVal)
            {
                interp.popEnv();
                return retVal.value ? *retVal.value : Nil{};
            }

            interp.popEnv();
            return Nil{};
        }
    };

    i.getCurrentEnv().define(std::get<std::string>(i.ctxt.getToken(s->name).value), callable);
}

void Interpreter::StmtEvalVisitor::operator()(const Return* s) const
{
    if (s->value)
        throw ReturnValue{i.eval(*s->value)};

    throw ReturnValue{std::nullopt};
}

void Interpreter::StmtEvalVisitor::operator()(const Block* s) const
{
    i.pushEnv(&i.getCurrentEnv());

    try
    {
        for (auto child : s->statements)
        {
            i.eval(child);
        }
    }
    catch(...)
    {
        // Return values are passed by exceptions, so we need to catch
        // them to ensure popping env. We should switch to a different
        // return mechanism.
        i.popEnv();
        throw;
    }

    i.popEnv();
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

void Interpreter::StmtEvalVisitor::operator()(const Unit* s) const
{
    for (auto stmt : s->statements)
        i.eval(stmt);
}

void Interpreter::collect()
{
    if (collectCounter > 10)
    {
        collectCounter = 0;
        std::unordered_set<Environment*> reached;
        std::vector<Environment*> exploring{&getCurrentEnv()};
        while(!exploring.empty())
        {
            auto env = exploring.back();
            exploring.pop_back();
            reached.insert(env);

            auto p = env->enclosing;
            while(p)
            {
                if (reached.contains(p))
                    break;
                reached.insert(p);
                exploring.push_back(p);
                p = p->enclosing;
            }

            for(auto& [_, val] : env->values)
            {
                if (auto callable = std::get_if<Callable>(&val))
                {
                    auto env = callable->closure;
                    if (reached.contains(env))
                        continue;
                    reached.insert(env);
                    exploring.push_back(env);
                }
            }
        }

        for(auto it = allEnvs.begin(); it != allEnvs.end();)
        {
            if (reached.contains(it->get()))
            {
                ++it;
                continue;
            }

            it = allEnvs.erase(it);
        }

    }
    ++collectCounter;
}

Environment* Interpreter::pushEnv(Environment *current)
{
    auto result = allEnvs.insert(std::make_unique<Environment>(current));
    stack.push_back(result.first->get());
    return result.first->get();
}

void Interpreter::popEnv()
{
    stack.pop_back();
}
