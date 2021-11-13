#include "include/parser.h"

#include <fmt/format.h>

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

#define BIND(var, x)  BIND_IMPL(var, x, CONCAT(__val, __COUNTER__))
#define BIND_IMPL(var, x, y)       \
  auto y = (x);                    \
  if (!y) return std::nullopt;     \
  auto var = *y

using enum TokenType;

std::optional<StatementIndex> Parser::declaration()
{
    // TODO: synchronize.
    if (match(VAR))
        return varDeclaration();

    return statement();
}

std::optional<Index<VarDecl>> Parser::varDeclaration()
{
    BIND(name, consume(IDENTIFIER, "Expect variable name."));

    std::optional<ExpressionIndex> init;
    if (match(EQUAL))
    {
        init = expression();
        if (!init)
            return std::nullopt;
    }

    consume(SEMICOLON, "Expect ';' after variable declaration.");
    return context.makeVarDecl(name, init);
}

std::optional<StatementIndex> Parser::statement()
{
    if (match(IF)) return ifStatement();
    if (match(PRINT)) return printStatement();
    if (match(WHILE)) return whileStatement();
    if (match(LEFT_BRACE)) return block();

    return expressionStatement();
}

std::optional<Index<IfStatement>> Parser::ifStatement()
{
    consume(LEFT_PAREN, "Expect '(' after if.");
    BIND(condition, expression());
    consume(RIGHT_PAREN, "Expect ')' after if condition.");

    BIND(thenBranch, statement());
    std::optional<StatementIndex> elseBranch;
    if (match(ELSE))
    {
        BIND(elseStmt, statement());
        elseBranch = elseStmt;
    }

    return context.makeIf(condition, thenBranch, elseBranch);
}

std::optional<Index<PrintStatement>> Parser::printStatement()
{
    BIND(value, expression());
    consume(SEMICOLON, "Expect ';' after value.");

    return context.makePrint(value);
}

std::optional<Index<WhileStatement>> Parser::whileStatement()
{
    consume(LEFT_PAREN, "Expect '(' after while.");
    BIND(condition, expression());
    consume(RIGHT_PAREN, "Expect ')' after while condition.");

    BIND(body, statement());

    return context.makeWhile(condition, body);
}

std::optional<Index<Block>> Parser::block()
{
    std::vector<StatementIndex> statements;

    while(!check(RIGHT_BRACE) && !isAtEnd())
    {
        BIND(stmt, declaration());
        statements.push_back(stmt);
    }

    consume(RIGHT_BRACE, "Expect '}' after block.");

    return context.makeBlock(std::move(statements));
}

std::optional<Index<ExprStatement>> Parser::expressionStatement()
{
    BIND(value, expression());
    consume(SEMICOLON, "Expect ';' after value.");

    return context.makeExprStmt(value);
}

std::optional<ExpressionIndex> Parser::expression()
{
    return assignment();
}

std::optional<ExpressionIndex> Parser::assignment()
{
    BIND(expr, equality());

    if (match(EQUAL))
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

    while(match(BANG_EQUAL, EQUAL_EQUAL))
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

    while(match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL))
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

    while(match(MINUS, PLUS))
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

    while(match(SLASH, STAR))
    {
        Index<Token> op = previous();
        BIND(right, unary());
        expr = context.makeBinary(expr, op, right);
    }

    return expr;
}

std::optional<ExpressionIndex> Parser::unary()
{
    if (match(BANG, MINUS))
    {
        Index<Token> op = previous();
        BIND(subExpr, unary());
        return context.makeUnary(op, subExpr);
    }

    return primary();
}

std::optional<ExpressionIndex> Parser::primary()
{
    if (match(FALSE))
        return context.makeLiteral(previous());
    if (match(TRUE))
        return context.makeLiteral(previous());
    if (match(NIL))
        return context.makeLiteral(previous());

    if (match(STRING, NUMBER))
        return context.makeLiteral(previous());

    if (match(IDENTIFIER))
        return context.makeDeclRef(previous());

    if (match(LEFT_PAREN))
    {
        Index<Token> begin = previous();
        BIND(expr, expression());
        consume(RIGHT_PAREN, "Expect ')' after expression");
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
        if (context.getToken(previous()).type == SEMICOLON)
            return;

        switch(context.getToken(peek()).type)
        {
            case CLASS:
            case FUN:
            case VAR:
            case FOR:
            case IF:
            case WHILE:
            case PRINT:
            case RET:
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
    if (t.type == END_OF_FILE)
    {
        report(t.line, " at end", std::move(message));
    }
    else
    {
        report(t.line, fmt::format(" at '{}'", print(t)), std::move(message));
    }
}
