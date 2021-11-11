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
struct Block;
struct IfStatement;
struct WhileStatement;

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
                               const VarDecl*, const Block*,
                               const IfStatement*, const WhileStatement*>;

using ExpressionIndex = std::variant<Index<Binary>, Index<Assign>,
                                     Index<Unary>, Index<Literal>,
                                     Index<Grouping>, Index<DeclRef>>;
using StatementIndex = std::variant<Index<ExprStatement>, Index<PrintStatement>,
                                    Index<VarDecl>, Index<Block>,
                                    Index<IfStatement>, Index<WhileStatement>>;

struct Binary
{
    Index<Token> op;
    ExpressionIndex left, right;

    // TODO: remove once compiler is standard conforming.
    Binary(Index<Token> op, ExpressionIndex left, ExpressionIndex right)
        : op(op), left(left), right(right) {}
};

struct Assign
{
    Index<Token> name;
    ExpressionIndex value;

    Assign(Index<Token> name, ExpressionIndex value)
        : name(name), value(value) {}
};

struct Unary
{
    Index<Token> op;
    ExpressionIndex subExpr;

    Unary(Index<Token> op, ExpressionIndex subExpr) : op(op), subExpr(subExpr) {}
};

struct Literal
{
    Index<Token> value;

    Literal (Index<Token> value) : value(value) {}
};

struct Grouping
{
    Index<Token> begin;
    Index<Token> end;
    ExpressionIndex subExpr;

    Grouping(Index<Token> begin, Index<Token> end, ExpressionIndex subExpr)
        : begin(begin), end(end), subExpr(subExpr) {}
};

struct DeclRef
{
    Index<Token> name;

    DeclRef(Index<Token> name) : name(name) {}
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
    Index<Token> name;
    std::optional<ExpressionIndex> init;

    VarDecl(Index<Token> name, std::optional<ExpressionIndex> init) : name(name), init(init) {}
};

struct Block
{
    std::vector<StatementIndex> statements;

    Block(std::vector<StatementIndex> statements) : statements(std::move(statements)) {}
};

struct IfStatement
{
    ExpressionIndex condition;
    StatementIndex thenBranch;
    std::optional<StatementIndex> elseBranch;

    IfStatement(ExpressionIndex condition, StatementIndex thenBranch, std::optional<StatementIndex> elseBranch)
        : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}
};

struct WhileStatement
{
    ExpressionIndex condition;
    StatementIndex body;

    WhileStatement(ExpressionIndex condition, StatementIndex body)
        : condition(condition), body(body) {}
};

class ASTContext
{
public:
    // Expression factories.
    Index<Binary> makeBinary(ExpressionIndex left, Index<Token> t, ExpressionIndex right)
    {
        return insert_node(binaries, t, left, right);
    }

    Index<Assign> makeAssign(Index<Token> name, ExpressionIndex value)
    {
        return insert_node(assignments, name, value);
    }

    Index<Unary> makeUnary(Index<Token> t, ExpressionIndex subExpr)
    {
        return insert_node(unaries, t, subExpr);
    }

    Index<Literal> makeLiteral(Index<Token> t)
    {
        return insert_node(literals, t);
    }

    Index<Grouping> makeGrouping(Index<Token> begin, ExpressionIndex subExpr, Index<Token> end)
    {
        return insert_node(groupings, begin, end, subExpr);
    }

    Index<DeclRef> makeDeclRef(Index<Token> name)
    {
        return insert_node(declRefs, name);
    }

    // Statement factories.
    Index<PrintStatement> makePrint(ExpressionIndex subExpr)
    {
        return insert_node(prints, subExpr);
    }

    Index<ExprStatement> makeExprStmt(ExpressionIndex subExpr)
    {
        return insert_node(exprStmts, subExpr);
    }

    Index<VarDecl> makeVarDecl(Index<Token> name, std::optional<ExpressionIndex> init)
    {
        return insert_node(varDecls, name, init);
    }

    Index<Block> makeBlock(std::vector<StatementIndex> statements)
    {
        return insert_node(blocks, std::move(statements));
    }

    Index<IfStatement> makeIf(ExpressionIndex condition, StatementIndex thenBranch, std::optional<StatementIndex> elseBranch)
    {
        return insert_node(ifs, condition, thenBranch, elseBranch);
    }

    Index<WhileStatement> makeWhile(ExpressionIndex condition, StatementIndex body)
    {
        return insert_node(whiles, condition, body);
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

    const Token& getToken(Index<Token> idx) const
    {
        return tokens[idx.id];
    }

    void addTokens(std::vector<Token>&& newTokens)
    {
        // Get rid if the now incorrect end of file token.
        if (!tokens.empty())
            tokens.pop_back();

        tokens.insert(tokens.end(), std::make_move_iterator(newTokens.begin()),
                      std::make_move_iterator(newTokens.end()));
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
    std::vector<Block>            blocks;
    std::vector<IfStatement>      ifs;
    std::vector<WhileStatement>   whiles;

    std::vector<Token>            tokens;

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
        auto operator()(Index<Block> index) const            -> Statement { return &ctx.blocks[index.id]; }
        auto operator()(Index<IfStatement> index) const      -> Statement { return &ctx.ifs[index.id]; }
        auto operator()(Index<WhileStatement> index) const   -> Statement { return &ctx.whiles[index.id]; }
    } statementNodeGetter{*this};


    template<typename NodeContainer, typename... Args>
    Index<typename NodeContainer::value_type> insert_node(NodeContainer& c, Args&&... args)
    {
        c.emplace_back(std::forward<Args>(args)...);
        return c.size() - 1;
    }
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
        std::string operator()(const Block* s) const;
        std::string operator()(const IfStatement* s) const;
        std::string operator()(const WhileStatement* s) const;
    } stmtVisitor{*this};
};

#endif
