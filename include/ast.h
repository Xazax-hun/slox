#ifndef AST_H
#define AST_H

#include <optional>
#include <variant>
#include <vector>
#include <functional>

#include "include/lexer.h"

// Expressions.
struct Binary;
struct Assign;
struct Unary;
struct Literal;
struct Grouping;
struct DeclRef;
struct Call;

// Statements.
struct ExprStatement;
struct PrintStatement;
struct VarDecl;
struct FunDecl;
struct Block;
struct IfStatement;
struct WhileStatement;
struct Return;
struct Unit;

template<typename T>
struct Index
{
    using type = T;
    unsigned id; 
};

template<typename T>
struct std::hash<Index<T>>
{
    std::size_t operator()(Index<T> index) const noexcept
    {
        return std::hash<unsigned>{}(index.id);
    }
};

template<typename T>
bool operator==(Index<T> lhs, Index<T> rhs) noexcept { return lhs.id == rhs.id; }

using Expression = std::variant<const Binary*, const Assign*,
                                const Unary*, const Literal*,
                                const Grouping*, const DeclRef*,
                                const Call*>;
using Statement = std::variant<const ExprStatement*, const PrintStatement*,
                               const VarDecl*, const Block*,
                               const IfStatement*, const WhileStatement*,
                               const FunDecl*, const Return*,
                               const Unit*>;

using ExpressionIndex = std::variant<Index<Binary>, Index<Assign>,
                                     Index<Unary>, Index<Literal>,
                                     Index<Grouping>, Index<DeclRef>,
                                     Index<Call>>;
using StatementIndex = std::variant<Index<ExprStatement>, Index<PrintStatement>,
                                    Index<VarDecl>, Index<Block>,
                                    Index<IfStatement>, Index<WhileStatement>,
                                    Index<FunDecl>, Index<Return>,
                                    Index<Unit>>;

struct Binary
{
    Index<Token> op;
    ExpressionIndex left, right;
};

struct Assign
{
    Index<Token> name;
    ExpressionIndex value;
};

struct Unary
{
    Index<Token> op;
    ExpressionIndex subExpr;
};

struct Literal
{
    Index<Token> value;
};

struct Grouping
{
    Index<Token> begin;
    Index<Token> end;
    ExpressionIndex subExpr;
};

struct DeclRef
{
    Index<Token> name;
};

struct Call
{
    ExpressionIndex callee;
    Index<Token> open;
    std::vector<ExpressionIndex> args;
    Index<Token> close;
};

struct ExprStatement
{
    ExpressionIndex subExpr;
};

struct PrintStatement
{
    ExpressionIndex subExpr;
};

struct VarDecl
{
    Index<Token> name;
    std::optional<ExpressionIndex> init;
};

struct FunDecl
{
    Index<Token> name;
    std::vector<Index<Token>> params;
    std::vector<StatementIndex> body;
};

struct Return
{
    Index<Token> keyword;
    std::optional<ExpressionIndex> value;
};

struct Block
{
    std::vector<StatementIndex> statements;
};

struct IfStatement
{
    ExpressionIndex condition;
    StatementIndex thenBranch;
    std::optional<StatementIndex> elseBranch;
};

struct WhileStatement
{
    ExpressionIndex condition;
    StatementIndex body;
};

struct Unit
{
    std::vector<StatementIndex> statements;
};

class ASTContext
{
public:
    // Expression factories.
    Index<Binary> makeBinary(ExpressionIndex left, Index<Token> t, ExpressionIndex right) noexcept
    {
        return insert_node(binaries, t, left, right);
    }

    Index<Assign> makeAssign(Index<Token> name, ExpressionIndex value) noexcept
    {
        return insert_node(assignments, name, value);
    }

    Index<Unary> makeUnary(Index<Token> t, ExpressionIndex subExpr) noexcept
    {
        return insert_node(unaries, t, subExpr);
    }

    Index<Literal> makeLiteral(Index<Token> t) noexcept
    {
        return insert_node(literals, t);
    }

    Index<Grouping> makeGrouping(Index<Token> begin, ExpressionIndex subExpr, Index<Token> end) noexcept
    {
        return insert_node(groupings, begin, end, subExpr);
    }

    Index<DeclRef> makeDeclRef(Index<Token> name) noexcept
    {
        return insert_node(declRefs, name);
    }

    Index<Call> makeCall(ExpressionIndex callee, Index<Token> begin,
                         std::vector<ExpressionIndex>&& args, Index<Token> end) noexcept
    {
        return insert_node(calls, callee, begin, std::move(args), end);
    }

    // Statement factories.
    Index<PrintStatement> makePrint(ExpressionIndex subExpr) noexcept
    {
        return insert_node(prints, subExpr);
    }

    Index<ExprStatement> makeExprStmt(ExpressionIndex subExpr) noexcept
    {
        return insert_node(exprStmts, subExpr);
    }

    Index<VarDecl> makeVarDecl(Index<Token> name, std::optional<ExpressionIndex> init) noexcept
    {
        return insert_node(varDecls, name, init);
    }

    Index<FunDecl> makeFunDecl(Index<Token> name, std::vector<Index<Token>>&& params, std::vector<StatementIndex>&& body) noexcept
    {
        return insert_node(funDecls, name, std::move(params), std::move(body));
    }

    Index<Return> makeReturn(Index<Token> keyword, std::optional<ExpressionIndex> value) noexcept
    {
        return insert_node(returns, keyword, value);
    }

    Index<Block> makeBlock(std::vector<StatementIndex>&& statements) noexcept
    {
        return insert_node(blocks, std::move(statements));
    }

    Index<IfStatement> makeIf(ExpressionIndex condition, StatementIndex thenBranch, std::optional<StatementIndex> elseBranch) noexcept
    {
        return insert_node(ifs, condition, thenBranch, elseBranch);
    }

    Index<WhileStatement> makeWhile(ExpressionIndex condition, StatementIndex body) noexcept
    {
        return insert_node(whiles, condition, body);
    }

    Index<Unit> makeUnit(std::vector<StatementIndex>&& statements) noexcept
    {
        return insert_node(units, std::move(statements));
    }
    
    // Getters.
    Expression getNode(ExpressionIndex idx) const noexcept
    {
        return std::visit(exprNodeGetter, idx);
    }

    Statement getNode(StatementIndex idx) const noexcept
    {
        return std::visit(statementNodeGetter, idx);
    }

    const Token& getToken(Index<Token> idx) const noexcept
    {
        return tokens[idx.id];
    }

    const TokenList& getTokenList() const noexcept
    {
        return tokens;
    }

    void addTokens(TokenList&& newTokens) noexcept
    {
        tokens.mergeTokensFrom(std::move(newTokens));
    }

private:
    // Expressions.
    std::vector<Binary>   binaries;
    std::vector<Assign>   assignments;
    std::vector<Unary>    unaries;
    std::vector<Literal>  literals;
    std::vector<Grouping> groupings;
    std::vector<DeclRef>  declRefs;
    std::vector<Call>     calls;

    // Statements.
    std::vector<PrintStatement>   prints;
    std::vector<ExprStatement>    exprStmts;
    std::vector<VarDecl>          varDecls;
    std::vector<FunDecl>          funDecls;
    std::vector<Return>           returns;
    std::vector<Block>            blocks;
    std::vector<IfStatement>      ifs;
    std::vector<WhileStatement>   whiles;
    std::vector<Unit>             units;

    TokenList                     tokens;

    struct GetExprNode
    {
        const ASTContext& ctx;
        auto operator()(Index<Binary> index) const noexcept   -> Expression { return &ctx.binaries[index.id]; }
        auto operator()(Index<Assign> index) const noexcept   -> Expression { return &ctx.assignments[index.id]; }
        auto operator()(Index<Unary> index) const noexcept    -> Expression { return &ctx.unaries[index.id]; }
        auto operator()(Index<Literal> index) const noexcept  -> Expression { return &ctx.literals[index.id]; }
        auto operator()(Index<Grouping> index) const noexcept -> Expression { return &ctx.groupings[index.id]; }
        auto operator()(Index<DeclRef> index) const noexcept  -> Expression { return &ctx.declRefs[index.id]; }
        auto operator()(Index<Call> index) const noexcept     -> Expression { return &ctx.calls[index.id]; }
    } exprNodeGetter{*this};

    struct GetStatementNode
    {
        const ASTContext& ctx;
        auto operator()(Index<PrintStatement> index) const noexcept  -> Statement { return &ctx.prints[index.id]; }
        auto operator()(Index<ExprStatement> index) const noexcept   -> Statement { return &ctx.exprStmts[index.id]; }
        auto operator()(Index<VarDecl> index) const noexcept         -> Statement { return &ctx.varDecls[index.id]; }
        auto operator()(Index<FunDecl> index) const noexcept         -> Statement { return &ctx.funDecls[index.id]; }
        auto operator()(Index<Return> index) const noexcept          -> Statement { return &ctx.returns[index.id]; }
        auto operator()(Index<Block> index) const noexcept           -> Statement { return &ctx.blocks[index.id]; }
        auto operator()(Index<IfStatement> index) const noexcept     -> Statement { return &ctx.ifs[index.id]; }
        auto operator()(Index<WhileStatement> index) const noexcept  -> Statement { return &ctx.whiles[index.id]; }
        auto operator()(Index<Unit> index) const noexcept            -> Statement { return &ctx.units[index.id]; }
    } statementNodeGetter{*this};


    template<typename NodeContainer, typename... Args>
    Index<typename NodeContainer::value_type> insert_node(NodeContainer& c, Args&&... args) noexcept
    {
        c.emplace_back(std::forward<Args>(args)...);
        return {static_cast<unsigned>(c.size()) - 1};
    }
};

class ASTPrinter
{
public:
    explicit ASTPrinter(const ASTContext& c) noexcept : c(c) {}
    std::string print(ExpressionIndex e) const noexcept;
    std::string print(StatementIndex e) const noexcept;

private:
    const ASTContext& c;
    struct ExprPrintVisitor
    {
        const ASTPrinter& printer;
        std::string operator()(const Binary* b) const noexcept;
        std::string operator()(const Assign* a) const noexcept;
        std::string operator()(const Unary* u) const noexcept;
        std::string operator()(const Literal* l) const noexcept;
        std::string operator()(const Grouping* l) const noexcept;
        std::string operator()(const DeclRef* r) const noexcept;
        std::string operator()(const Call* c) const noexcept;
    } exprVisitor{*this};

    struct StmtPrintVisitor
    {
        const ASTPrinter& printer;
        std::string operator()(const PrintStatement* s) const noexcept;
        std::string operator()(const ExprStatement* s) const noexcept;
        std::string operator()(const VarDecl* s) const noexcept;
        std::string operator()(const FunDecl* s) const noexcept;
        std::string operator()(const Return* s) const noexcept;
        std::string operator()(const Block* s) const noexcept;
        std::string operator()(const IfStatement* s) const noexcept;
        std::string operator()(const WhileStatement* s) const noexcept;
        std::string operator()(const Unit* s) const noexcept;
    } stmtVisitor{*this};
};

#endif
