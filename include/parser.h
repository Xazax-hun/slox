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
    explicit Parser(const DiagnosticEmitter& diag) noexcept : diag(diag) {}

    // Reentrant. Invoking again will continue parsing with the tokens
    // added since the last invocation.
    std::optional<Index<Unit>> parse();

    // Add the tokens without continuing the parsing.
    void addTokens(TokenList tokens);

    const ASTContext& getContext() const { return context; }

private:
    // Statements.
    std::optional<StatementIndex> declaration();
    std::optional<Index<FunDecl>> funDeclaration();
    std::optional<Index<VarDecl>> varDeclaration();
    std::optional<StatementIndex> statement();
    std::optional<StatementIndex> forStatement();
    std::optional<Index<IfStatement>> ifStatement();
    std::optional<Index<PrintStatement>> printStatement();
    std::optional<Index<Return>> returnStatement();
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

    // Helpers.
    std::optional<std::vector<StatementIndex>> statementList();
    std::optional<ExpressionIndex> finishCall(Index<Token> begin, ExpressionIndex callee);

    // Error recovery.
    void synchronize();

    // Utilities.
    Index<Token> peek() const noexcept { return {current}; }
    Index<Token> previous() const noexcept { return {current - 1}; }
    bool isAtEnd() const noexcept
    {
        return context.getToken(peek()).type == TokenType::END_OF_FILE;
    }

    bool check(TokenType type) const noexcept
    {
        if (isAtEnd()) return false;
        return context.getToken(peek()).type == type;
    }

    template<typename... T>
    bool match(T... tokenTypes) noexcept
    {
        bool b = (check(tokenTypes) || ...);
        if (b)
            advance();
        return b;
    }

    Index<Token> advance() noexcept
    {
        if (!isAtEnd()) ++current;
        return previous();
    }

    std::optional<Index<Token>> consume(TokenType type, std::string_view message) noexcept
    {
        if (check(type))
            return advance();

        error(peek(), message);
        return std::nullopt;
    }

    void error(Index<Token> t, std::string_view message) noexcept;

    ASTContext context;
    unsigned current = 0;
    const DiagnosticEmitter& diag;
};

#endif