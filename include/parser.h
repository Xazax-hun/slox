#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <optional>

#include "include/lexer.h"
#include "include/ast.h"

class Parser
{
public:
    std::optional<StatementIndex> parse(std::vector<Token> tokens)
    {
        context.addTokens(std::move(tokens));
        return declaration();
    }

    const ASTContext& getContext() const { return context; }

private:
    // Statements.
    std::optional<StatementIndex> declaration();
    std::optional<Index<VarDecl>> varDeclaration();
    std::optional<StatementIndex> statement();
    std::optional<Index<Block>> forStatement();
    std::optional<Index<IfStatement>> ifStatement();
    std::optional<Index<PrintStatement>> printStatement();
    std::optional<Index<WhileStatement>> whileStatement();
    std::optional<Index<Block>> block();
    std::optional<Index<ExprStatement>> expressionStatement();

    // Expressions.
    std::optional<ExpressionIndex> expression();
    std::optional<ExpressionIndex> assignment();
    std::optional<ExpressionIndex> or_();
    std::optional<ExpressionIndex> and_();
    std::optional<ExpressionIndex> equality();
    std::optional<ExpressionIndex> comparison();
    std::optional<ExpressionIndex> term();
    std::optional<ExpressionIndex> factor();
    std::optional<ExpressionIndex> unary();
    std::optional<ExpressionIndex> call();
    std::optional<ExpressionIndex> primary();

    std::optional<ExpressionIndex> finishCall(Index<Token> begin, ExpressionIndex callee);

    // Error recovery.
    void synchronize();

    // Utilities.
    Index<Token> peek() const { return {current}; }
    Index<Token> previous() const { return {current - 1}; }
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