#include "include/parser.h"

#include <fmt/format.h>

#define BIND(var, x)             \
  auto val = (x);                \
  if (!val) return std::nullopt; \
  auto var = *val

std::optional<ExpressionIndex> Parser::expression()
{
    return equality();
}

std::optional<ExpressionIndex> Parser::equality()
{
    BIND(expr, comparison());

    while(match(TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL))
    {
        Token op = previous();
        BIND(right, comparison());
        expr = context.makeBinary(expr, op, right);
    }

    return expr;
}

std::optional<ExpressionIndex> Parser::comparison()
{
    BIND(expr, term());

    while(match(TokenType::GREATER, TokenType::GREATER_EQUAL,
                TokenType::LESS, TokenType::LESS_EQUAL))
    {
        Token op = previous();
        BIND(right, term());
        expr = context.makeBinary(expr, op, right);
    }

    return expr;
}

std::optional<ExpressionIndex> Parser::term()
{
    BIND(expr, factor());

    while(match(TokenType::MINUS, TokenType::PLUS))
    {
        Token op = previous();
        BIND(right, factor());
        expr = context.makeBinary(expr, op, right);
    }

    return expr;
}

std::optional<ExpressionIndex> Parser::factor()
{
    BIND(expr, unary());

    while(match(TokenType::SLASH, TokenType::STAR))
    {
        Token op = previous();
        BIND(right, unary());
        expr = context.makeBinary(expr, op, right);
    }

    return expr;
}

std::optional<ExpressionIndex> Parser::unary()
{
    if (match(TokenType::BANG, TokenType::MINUS))
    {
        Token op = previous();
        BIND(subExpr, unary());
        return context.makeUnary(op, subExpr);
    }

    return primary();
}

std::optional<ExpressionIndex> Parser::primary()
{
    if (match(TokenType::FALSE))
        return context.makeLiteral(previous());
    if (match(TokenType::TRUE))
        return context.makeLiteral(previous());
    if (match(TokenType::NIL))
        return context.makeLiteral(previous());

    if (match(TokenType::STRING, TokenType::NUMBER))
        return context.makeLiteral(previous());

    if (match(TokenType::LEFT_PAREN))
    {
        Token begin = previous();
        BIND(expr, expression());
        consume(TokenType::RIGHT_PAREN, "Expect ')' after expression");
        Token end = previous();
        return context.makeGrouping(begin, expr, end);
    }

    // TODO: emit diagnostic.
    return std::nullopt;
}

void Parser::error(Token t, std::string message)
{
    if (t.type == TokenType::END_OF_FILE)
    {
        report(t.line, " at end", message);
    }
    else
    {
        report(t.line, fmt::format(" at '{}'", print(t)), message);
    }
}
