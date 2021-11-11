#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <optional>

#include "include/lexer.h"
#include "include/ast.h"
#include "include/utils.h"

class Parser
{
public:
    Parser(std::vector<Token> tokens) : context(std::move(tokens)) {}

    std::optional<StatementIndex> parse() { return declaration(); }

    const ASTContext& getContext() const { return context; }

private:
    // Statements.
    std::optional<StatementIndex> declaration();
    std::optional<Index<VarDecl>> varDeclaration();
    std::optional<StatementIndex> statement();
    std::optional<Index<PrintStatement>> printStatement();
    std::optional<Index<ExprStatement>> expressionStatement();

    // Expressions.
    std::optional<ExpressionIndex> expression();
    std::optional<ExpressionIndex> assignment();
    std::optional<ExpressionIndex> equality();
    std::optional<ExpressionIndex> comparison();
    std::optional<ExpressionIndex> term();
    std::optional<ExpressionIndex> factor();
    std::optional<ExpressionIndex> unary();
    std::optional<ExpressionIndex> primary();

    // Error recovery.
    void synchronize();

    // Utilities.
    Index<Token> peek() const { return current; }
    Index<Token> previous() const { return current - 1; }
    bool isAtEnd() const
    {
        return context.getToken(peek()).type == TokenType::END_OF_FILE;
    }

    bool check(TokenType type) const
    {
        if (isAtEnd()) return false;
        return context.getToken(peek()).type == type;
    }

    template<typename... T>
    bool match(T... tokenTypes)
    {
        bool b = (check(tokenTypes) || ...);
        if (b)
            advance();
        return b;
    }

    Index<Token> advance()
    {
        if (!isAtEnd()) ++current;
        return previous();
    }

    std::optional<Index<Token>> consume(TokenType type, std::string message)
    {
        if (check(type))
            return advance();

        error(peek(), std::move(message));
        return std::nullopt;
    }

    void error(Index<Token> t, std::string message);

    ASTContext context;
    unsigned current = 0;
};

#endif