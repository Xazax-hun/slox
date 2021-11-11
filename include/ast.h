#ifndef AST_H
#define AST_H

#include <optional>
#include <variant>
#include <vector>

#include "include/lexer.h"

// Expressions.
struct Binary;
struct Assign;
struct Unary;
struct Literal;
struct Grouping;
struct DeclRef;

// Statements.
struct ExprStatement;
struct PrintStatement;
struct VarDecl;

template<typename T>
struct Index
{
    using type = T;
    unsigned id; 
    constexpr Index(unsigned id) : id(id) {}
};

using Expression = std::variant<const Binary*, const Assign*,
                                const Unary*, const Literal*,
                                const Grouping*, const DeclRef*>;
using Statement = std::variant<const ExprStatement*, const PrintStatement*,
                               const VarDecl*>;

using ExpressionIndex = std::variant<Index<Binary>, Index<Assign>,
                                     Index<Unary>, Index<Literal>,
                                     Index<Grouping>, Index<DeclRef>>;
using StatementIndex = std::variant<Index<ExprStatement>, Index<PrintStatement>,
                                    Index<VarDecl>>;

struct Binary
{
    const Token& op;
    ExpressionIndex left, right;

    // TODO: remove once compiler is standard conforming.
    Binary(const Token& op, ExpressionIndex left, ExpressionIndex right)
        : op(op), left(left), right(right) {}
};

struct Assign
{
    const Token& name;
    ExpressionIndex value;

    Assign(const Token& name, ExpressionIndex value)
        : name(name), value(value) {}
};

struct Unary
{
    const Token& op;
    ExpressionIndex subExpr;

    Unary(const Token& op, ExpressionIndex subExpr) : op(op), subExpr(subExpr) {}
};

struct Literal
{
    const Token& value;

    Literal (const Token& value) : value(value) {}
};

struct Grouping
{
    const Token& begin;
    const Token& end;
    ExpressionIndex subExpr;

    Grouping(const Token& begin, const Token& end, ExpressionIndex subExpr)
        : begin(begin), end(end), subExpr(subExpr) {}
};

struct DeclRef
{
    const Token& name;

    DeclRef(const Token& name) : name(name) {}
};

struct ExprStatement
{
    ExpressionIndex subExpr;

    ExprStatement(ExpressionIndex subExpr) : subExpr(subExpr) {}
};

struct PrintStatement
{
    ExpressionIndex subExpr;

    PrintStatement(ExpressionIndex subExpr) : subExpr(subExpr) {}
};

struct VarDecl
{
    const Token& name;
    std::optional<ExpressionIndex> init;

    VarDecl(const Token& name, std::optional<ExpressionIndex> init) : name(name), init(init) {}
};

class ASTContext
{
public:
    // Expression factories.
    Index<Binary> makeBinary(ExpressionIndex left, const Token& t, ExpressionIndex right)
    {
        // TODO: replace the two lines boilerplate with a single line function call.
        binaries.emplace_back(t, left, right);
        return binaries.size() - 1;
    }

    Index<Binary> makeAssign(const Token& name, ExpressionIndex value)
    {
        assignments.emplace_back(name, value);
        return assignments.size() - 1;
    }

    Index<Unary> makeUnary(const Token& t, ExpressionIndex subExpr)
    {
        unaries.emplace_back(t, subExpr);
        return unaries.size() - 1;
    }

    Index<Literal> makeLiteral(const Token& t)
    {
        literals.emplace_back(t);
        return literals.size() - 1;
    }

    Index<Grouping> makeGrouping(const Token& begin, ExpressionIndex subExpr, const Token& end)
    {
        groupings.emplace_back(begin, end, subExpr);
        return groupings.size() - 1;
    }

    Index<DeclRef> makeDeclRef(const Token& name)
    {
        declRefs.emplace_back(name);
        return declRefs.size() - 1;
    }

    // Statement factories.
    Index<PrintStatement> makePrint(ExpressionIndex subExpr)
    {
        prints.emplace_back(subExpr);
        return prints.size() - 1;
    }

    Index<ExprStatement> makeExprStmt(ExpressionIndex subExpr)
    {
        exprStmts.emplace_back(subExpr);
        return exprStmts.size() - 1;
    }

    Index<VarDecl> makeVarDecl(const Token& name, std::optional<ExpressionIndex> init)
    {
        varDecls.emplace_back(name, init);
        return varDecls.size() - 1;
    }
    
    // Getters.
    Expression getNode(ExpressionIndex idx) const
    {
        return std::visit(exprNodeGetter, idx);
    }

    Statement getNode(StatementIndex idx) const
    {
        return std::visit(statementNodeGetter, idx);
    }

private:
    // Expressions.
    std::vector<Binary>   binaries;
    std::vector<Assign>   assignments;
    std::vector<Unary>    unaries;
    std::vector<Literal>  literals;
    std::vector<Grouping> groupings;
    std::vector<DeclRef>  declRefs;

    // Statements.
    std::vector<PrintStatement>   prints;
    std::vector<ExprStatement>    exprStmts;
    std::vector<VarDecl>          varDecls;

    struct GetExprNode
    {
        const ASTContext& ctx;
        auto operator()(Index<Binary> index) const   -> Expression { return &ctx.binaries[index.id]; }
        auto operator()(Index<Assign> index) const   -> Expression { return &ctx.assignments[index.id]; }
        auto operator()(Index<Unary> index) const    -> Expression { return &ctx.unaries[index.id]; }
        auto operator()(Index<Literal> index) const  -> Expression { return &ctx.literals[index.id]; }
        auto operator()(Index<Grouping> index) const -> Expression { return &ctx.groupings[index.id]; }
        auto operator()(Index<DeclRef> index) const ->  Expression { return &ctx.declRefs[index.id]; }
    } exprNodeGetter{*this};

    struct StatementExprNode
    {
        const ASTContext& ctx;
        auto operator()(Index<PrintStatement> index) const   -> Statement { return &ctx.prints[index.id]; }
        auto operator()(Index<ExprStatement> index) const    -> Statement { return &ctx.exprStmts[index.id]; }
        auto operator()(Index<VarDecl> index) const          -> Statement { return &ctx.varDecls[index.id]; }
    } statementNodeGetter{*this};
};

class ASTPrinter
{
public:
    ASTPrinter(const ASTContext& c) : c(c) {}
    std::string print(ExpressionIndex e) const;
    std::string print(StatementIndex e) const;

private:
    const ASTContext& c;
    struct ExprPrintVisitor
    {
        const ASTPrinter& printer;
        std::string operator()(const Binary* b) const;
        std::string operator()(const Assign* a) const;
        std::string operator()(const Unary* u) const;
        std::string operator()(const Literal* l) const;
        std::string operator()(const Grouping* l) const;
        std::string operator()(const DeclRef* l) const;
    } exprVisitor{*this};

    struct StmtPrintVisitor
    {
        const ASTPrinter& printer;
        std::string operator()(const PrintStatement* s) const;
        std::string operator()(const ExprStatement* s) const;
        std::string operator()(const VarDecl* s) const;
    } stmtVisitor{*this};
};

#endif
