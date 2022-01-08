#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <include/ast.h>
#include <include/utils.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <optional>

struct CompileTimeError
{
    Index<Token> where;
    std::string message;
};

using Resolution = std::unordered_map<ExpressionIndex, int>;

// Resolve names to declarations.
class NameResolver
{
public:
    NameResolver(const ASTContext& ctxt, const DiagnosticEmitter& diag) noexcept
        : ctxt(ctxt), diag(diag) {}
    std::optional<Resolution> resolveVariables(StatementIndex stmt) noexcept;

private:
    void resolve(ExpressionIndex expr);
    void resolve(StatementIndex stmt);
    void resolveLocal(ExpressionIndex expr, const std::string& name);
    void resolveStatements(const std::vector<StatementIndex>& statements);

    void declare(Index<Token> tok);
    void define(Index<Token> tok);
    
    void beginScope();
    void endScope();

    struct ExprResolveVisitor
    {
        NameResolver& r;
        void operator()(const Binary* b) const;
        void operator()(const Assign* a) const;
        void operator()(const Unary* u) const;
        void operator()(const Literal*) const {} // No-op.
        void operator()(const Grouping* l) const;
        void operator()(const DeclRef* r) const;
        void operator()(const Call* c) const;
    } exprVisitor{*this};

    struct StmtResolveVisitor
    {
        NameResolver& r;
        void operator()(const PrintStatement* s) const;
        void operator()(const ExprStatement* s) const;
        void operator()(const VarDecl* v) const;
        void operator()(const FunDecl* f) const;
        void operator()(const Return* s) const;
        void operator()(const Block* s) const;
        void operator()(const IfStatement* s) const;
        void operator()(const WhileStatement* s) const;
        void operator()(const Unit* s) const;
    } stmtVisitor{*this};

    const ASTContext& ctxt;
    const DiagnosticEmitter& diag;

    using Scope = std::unordered_map<std::string, bool>;
    std::vector<Scope> stack;
    Resolution resolution;
    ExpressionIndex currentExpr{};
    bool isInFunction = false;
};

#endif