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
    Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

    std::optional<ExpressionIndex> parse() { return expression(); }

    const ASTContext& getContext() const { return context; }

private:
    std::optional<ExpressionIndex> expression();
    std::optional<ExpressionIndex> equality();
    std::optional<ExpressionIndex> comparison();
    std::optional<ExpressionIndex> term();
    std::optional<ExpressionIndex> factor();
    std::optional<ExpressionIndex> unary();
    std::optional<ExpressionIndex> primary();

    void synchronize();

    const Token& peek() const { return tokens[current]; }
    const Token& previous() const { return tokens[current - 1]; }
    bool isAtEnd() const { return peek().type == TokenType::END_OF_FILE; }

    bool check(TokenType type) const
    {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    template<typename... T>
    bool match(T... tokenTypes)
    {
        bool b = (check(tokenTypes) || ...);
        if (b)
            advance();
        return b;
    }

    const Token& advance()
    {
        if (!isAtEnd()) ++current;
        return previous();
    }

    std::optional<const Token*> consume(TokenType type, std::string message)
    {
        if (check(type))
            return &advance();

        error(peek(), std::move(message));
        return std::nullopt;
    }

    void error(const Token& t, std::string message);

    std::vector<Token> tokens;
    ASTContext context;
    unsigned current = 0;
};

#endif