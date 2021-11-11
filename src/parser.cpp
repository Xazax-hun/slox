#include "include/parser.h"

#include <fmt/format.h>

#define BIND(var, x)             \
  auto val = (x);                \
  if (!val) return std::nullopt; \
  auto var = *val

std::optional<StatementIndex> Parser::declaration()
{
    // TODO: synchronize.
    if (match(TokenType::VAR))
        return varDeclaration();

    return statement();
}

std::optional<Index<VarDecl>> Parser::varDeclaration()
{
    BIND(name, consume(TokenType::IDENTIFIER, "Expect variable name."));

    std::optional<ExpressionIndex> init;
    if (match(TokenType::EQUAL))
    {
        init = expression();
        if (!init)
            return std::nullopt;
    }

    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return context.makeVarDecl(name, init);
}

std::optional<StatementIndex> Parser::statement()
{
    if (match(TokenType::PRINT)) return printStatement();
    if (match(TokenType::LEFT_BRACE)) return block();

    return expressionStatement();
}

std::optional<Index<PrintStatement>> Parser::printStatement()
{
    BIND(value, expression());
    consume(TokenType::SEMICOLON, "Expect ';' after value.");

    return context.makePrint(value);
}

std::optional<Index<Block>> Parser::block()
{
    std::vector<StatementIndex> statements;

    while(!check(TokenType::RIGHT_BRACE) && !isAtEnd())
    {
        BIND(stmt, declaration());
        statements.push_back(stmt);
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");

    return context.makeBlock(std::move(statements));
}

std::optional<Index<ExprStatement>> Parser::expressionStatement()
{
    BIND(value, expression());
    consume(TokenType::SEMICOLON, "Expect ';' after value.");

    return context.makeExprStmt(value);
}

std::optional<ExpressionIndex> Parser::expression()
{
    return assignment();
}

std::optional<ExpressionIndex> Parser::assignment()
{
    BIND(expr, equality());

    if (match(TokenType::EQUAL))
    {
        Index<Token> equals = previous();
        BIND(value, assignment());

        if (const auto* dRefId = get_if<Index<DeclRef>>(&expr))
        {
            // TODO: simplify this pattern.
            auto dRefNode = std::get<const DeclRef*>(context.getNode(*dRefId));
            return context.makeAssign(dRefNode->name, value);
        }

        error(equals, "Invalid assignment target");
    }

    return expr;
}

std::optional<ExpressionIndex> Parser::equality()
{
    BIND(expr, comparison());

    while(match(TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL))
    {
        Index<Token> op = previous();
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
        Index<Token> op = previous();
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
        Index<Token> op = previous();
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
        Index<Token> op = previous();
        BIND(right, unary());
        expr = context.makeBinary(expr, op, right);
    }

    return expr;
}

std::optional<ExpressionIndex> Parser::unary()
{
    if (match(TokenType::BANG, TokenType::MINUS))
    {
        Index<Token> op = previous();
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

    if (match(TokenType::IDENTIFIER))
        return context.makeDeclRef(previous());

    if (match(TokenType::LEFT_PAREN))
    {
        Index<Token> begin = previous();
        BIND(expr, expression());
        consume(TokenType::RIGHT_PAREN, "Expect ')' after expression");
        Index<Token> end = previous();
        return context.makeGrouping(begin, expr, end);
    }

    error(peek(), "Unexpected token.");
    return std::nullopt;
}

void Parser::synchronize()
{
    advance();

    while(!isAtEnd())
    {
        if (context.getToken(previous()).type == TokenType::SEMICOLON)
            return;

        switch(context.getToken(peek()).type)
        {
            case TokenType::CLASS:
            case TokenType::FUN:
            case TokenType::VAR:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::PRINT:
            case TokenType::RETURN:
                return;
            
            default:
                break;
        }

        advance();
    }
}

void Parser::error(Index<Token> tIdx, std::string message)
{
    const Token& t = context.getToken(tIdx);
    if (t.type == TokenType::END_OF_FILE)
    {
        report(t.line, " at end", std::move(message));
    }
    else
    {
        report(t.line, fmt::format(" at '{}'", print(t)), std::move(message));
    }
}
