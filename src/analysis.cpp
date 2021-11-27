#include <include/analysis.h>

#include <fmt/format.h>

#include <include/utils.h>

// TODO: implement more analyses:
// * Unreachable code after return
// * Unused variable
// * Definitive initialization?
// * After break is implemented: check whether it is inside a loop.

std::optional<Resolution> NameResolver::resolveVariables(StatementIndex stmt)
{
    try
    {
        resolve(stmt);
        return resolution;
    }
    catch(const CompileTimeError& e)
    {
        error(ctxt.getToken(e.where).line, e.message);
        return std::nullopt;
    }
}

void NameResolver::resolve(ExpressionIndex expr)
{
    currentExpr = expr;
    auto node = ctxt.getNode(expr);
    std::visit(exprVisitor, node);
}

void NameResolver::resolve(StatementIndex stmt)
{
    auto node = ctxt.getNode(stmt);
    std::visit(stmtVisitor, node);
}

void NameResolver::resolveLocal(ExpressionIndex expr, const std::string& name)
{
    for(int i = static_cast<int>(stack.size()) - 1; i >= 0; --i)
    {
        if (auto it = stack[i].find(name); it != stack[i].end())
        {
            resolution.insert(std::make_pair(expr, stack.size() - 1 - i));
            return;
        }
    }
}

void NameResolver::resolveStatements(const std::vector<StatementIndex>& statements)
{
    for(auto stmt : statements)
        resolve(stmt);
}

void NameResolver::beginScope()
{
    stack.emplace_back();
}

void NameResolver::endScope()
{
    stack.pop_back();
}

void NameResolver::declare(Index<Token> tok)
{
    if (stack.empty())
        return;

    const auto& token = ctxt.getToken(tok);
    auto result = stack.back().insert(std::make_pair(std::get<std::string>(token.value),
                                                     false));
    if (!result.second)
    {
        const auto& token = ctxt.getToken(tok);
        const auto& name = std::get<std::string>(token.value);
        throw CompileTimeError{tok, fmt::format("Already a variable with name '{}' in this scope.", name)};
    }
}

void NameResolver::define(Index<Token> tok)
{
    if (stack.empty())
        return;

    const auto& token = ctxt.getToken(tok);
    stack.back().insert_or_assign(std::get<std::string>(token.value),
                                  true);
}

void NameResolver::StmtResolveVisitor::operator()(const Block* s) const
{
    r.beginScope();
    r.resolveStatements(s->statements);
    r.endScope();
}

void NameResolver::StmtResolveVisitor::operator()(const VarDecl* v) const
{
    r.declare(v->name);
    if (v->init)
        r.resolve(*v->init);
    r.define(v->name);
}

void NameResolver::StmtResolveVisitor::operator()(const FunDecl* f) const
{
    bool wasInFunction = r.isInFunction; // TODO: RAII.
    r.isInFunction = true;

    r.declare(f->name);
    r.define(f->name);

    r.beginScope();

    for(auto tok : f->params)
    {
        r.declare(tok);
        r.define(tok);
    }

    r.resolveStatements(f->body);

    r.endScope();

    r.isInFunction = wasInFunction;
}

void NameResolver::StmtResolveVisitor::operator()(const PrintStatement* s) const
{
    r.resolve(s->subExpr);
}

void NameResolver::StmtResolveVisitor::operator()(const ExprStatement* s) const
{
    r.resolve(s->subExpr);
}

void NameResolver::StmtResolveVisitor::operator()(const Return* s) const
{
    if (!r.isInFunction)
        throw CompileTimeError{s->keyword, "Can't return from top level code"};

    if (s->value)
        r.resolve(*s->value);
}

void NameResolver::StmtResolveVisitor::operator()(const IfStatement* s) const
{
    r.resolve(s->condition);
    r.resolve(s->thenBranch);
    if (s->elseBranch)
        r.resolve(*s->elseBranch);
}

void NameResolver::StmtResolveVisitor::operator()(const WhileStatement* s) const
{
    r.resolve(s->condition);
    r.resolve(s->body);
}

void NameResolver::StmtResolveVisitor::operator()(const Unit* s) const
{
    r.resolveStatements(s->statements);
}

void NameResolver::ExprResolveVisitor::operator()(const DeclRef* ref) const
{
    const auto& token = r.ctxt.getToken(ref->name);
    const auto& name = std::get<std::string>(token.value);
    
    if (!r.stack.empty())
    {
        auto it = r.stack.back().find(name);
        if (it != r.stack.back().end() && !it->second)
            throw CompileTimeError{ref->name, "Can't read local variable in its own initializer."};
    }

    r.resolveLocal(r.currentExpr, name);
}

void NameResolver::ExprResolveVisitor::operator()(const Assign* a) const
{
    const auto& token = r.ctxt.getToken(a->name);
    const auto& name = std::get<std::string>(token.value);

    r.resolve(a->value);
    r.resolveLocal(r.currentExpr, name);
}

void NameResolver::ExprResolveVisitor::operator()(const Binary* b) const
{
    r.resolve(b->left);
    r.resolve(b->right);
}

void NameResolver::ExprResolveVisitor::operator()(const Unary* u) const
{
    r.resolve(u->subExpr);
}

void NameResolver::ExprResolveVisitor::operator()(const Grouping* l) const
{
    r.resolve(l->subExpr);
}

void NameResolver::ExprResolveVisitor::operator()(const Call* c) const
{
    r.resolve(c->callee);

    for (auto arg : c->args)
    {
        r.resolve(arg);
    }
}
