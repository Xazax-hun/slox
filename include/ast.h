#ifndef AST_H
#define AST_H

#include <variant>
#include <vector>

#include "include/lexer.h"

struct Binary;
struct Unary;
struct Literal;
struct Grouping;
struct ASTContext;

template<typename T>
struct Index {
    using type = T;
    unsigned id; 
    constexpr Index(unsigned id) : id(id) {}
};

using Expression = std::variant<const Binary*, const Unary*,
                                const Literal*, const Grouping*>;

using ExpressionIndex = std::variant<Index<Binary>, Index<Unary>,
                                     Index<Literal>, Index<Grouping>>;

struct Binary
{
    Token op;
    ExpressionIndex left, right;

    // TODO: remove once compiler is standard conforming.
    Binary(Token op, ExpressionIndex left, ExpressionIndex right) : op(op), left(left), right(right) {}
};

struct Unary
{
    Token op;
    ExpressionIndex subExpr;

    Unary(Token op, ExpressionIndex subExpr) : op(op), subExpr(subExpr) {}
};

struct Literal
{
    Token value;

    Literal (Token value) : value(value) {}
};

struct Grouping
{
    Token begin;
    Token end;
    ExpressionIndex subExpr;

    Grouping(Token begin, Token end, ExpressionIndex subExpr) : begin(begin), end(end), subExpr(subExpr) {}
};

class ASTContext
{
public:
    Index<Binary> makeBinary(ExpressionIndex left, Token t, ExpressionIndex right)
    {
        binaries.emplace_back(t, left, right);
        return binaries.size() - 1;
    }

    Index<Unary> makeUnary(Token t, ExpressionIndex subExpr)
    {
        unaries.emplace_back(t, subExpr);
        return unaries.size() - 1;
    }

    Index<Literal> makeLiteral(Token t)
    {
        literals.emplace_back(t);
        return literals.size() - 1;
    }

    Index<Grouping> makeGrouping(Token begin, ExpressionIndex subExpr, Token end)
    {
        groupings.emplace_back(begin, end, subExpr);
        return groupings.size() - 1;
    }
    
    Expression getNode(ExpressionIndex idx) const
    {
        return std::visit(nodeGetter, idx);
    }

private:
    std::vector<Binary>   binaries;
    std::vector<Unary>    unaries;
    std::vector<Literal>  literals;
    std::vector<Grouping> groupings;

    struct GetNode
    {
        const ASTContext& ctx;
        auto operator()(Index<Binary> index) const   -> Expression { return &ctx.binaries[index.id]; }
        auto operator()(Index<Unary> index) const    -> Expression { return &ctx.unaries[index.id]; }
        auto operator()(Index<Literal> index) const  -> Expression { return &ctx.literals[index.id]; }
        auto operator()(Index<Grouping> index) const -> Expression { return &ctx.groupings[index.id]; }
    } nodeGetter{*this};
};

class ASTPrinter
{
public:
    ASTPrinter(const ASTContext& c) : c(c) {}
    std::string print(ExpressionIndex e) const;

private:
    const ASTContext& c;
    struct PrintVisitor
    {
        const ASTPrinter& printer;
        std::string operator()(const Binary* b) const;
        std::string operator()(const Unary* u) const;
        std::string operator()(const Literal* l) const;
        std::string operator()(const Grouping* l) const;
    } visitor{*this};
};

#endif
