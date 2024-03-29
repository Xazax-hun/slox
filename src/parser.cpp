#include "include/parser.h"

#include "include/utils.h"

#include <fmt/format.h>

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

#define BIND(var, x)  BIND_IMPL(var, x, CONCAT(__val, __COUNTER__))
#define BIND_IMPL(var, x, y)       \
  auto y = (x);                    \
  if (!y) return std::nullopt;     \
  auto var = *y

#define MUST_SUCCEED(x)  MUST_SUCCEED_IMPL(x, CONCAT(__val, __COUNTER__))
#define MUST_SUCCEED_IMPL(x, y)       \
  auto y = (x);                       \
  if (!y) return std::nullopt; 

using enum TokenType;

void Parser::addTokens(TokenList tokens)
{
    if (current == 0)
        current = tokens.getFirstSourceTokenIdx();

    context.addTokens(std::move(tokens));
}

// Entry point to parsing.
std::optional<Index<Unit>> Parser::parse()
{
    std::vector<StatementIndex> statements;
    while(!isAtEnd())
    {
        BIND(stmt, declaration());
        statements.push_back(stmt);
    }

    // TODO: in interactive mode, append to existing unit instead
    //       of creating a new one.
    return context.makeUnit(std::move(statements));
}

std::optional<StatementIndex> Parser::declaration()
{
    std::optional<StatementIndex> result;
    if (match(FUN))
        result = funDeclaration();
    else if (match(VAR))
        result = varDeclaration();
    else
        result = statement();

    if (!result)
    {
        // Skip to the next statement.
        synchronize();
        if (isAtEnd())
            return std::nullopt;
        // Continue parsing if there is code left.
        declaration();
        // Make sure we fail the parser.
        // TODO: can we do this iteratively?
        return std::nullopt;
    }

    return result;
}

// TODO: support methods.
std::optional<Index<FunDecl>> Parser::funDeclaration()
{
    BIND(name, consume(IDENTIFIER, "Expect function name."));
    MUST_SUCCEED(consume(LEFT_PAREN, "Expect '(' after function name."));
    std::vector<Index<Token>> params;
    if (!check(RIGHT_PAREN))
    {
        do
        {
            BIND(param, consume(IDENTIFIER, "Expect parameter name."));
            params.push_back(param);
        } while (match(COMMA));

        if (params.size() >= 255)
        {
            error(peek(), "Can't have more than 255 parameters.");
            return std::nullopt;
        }
    }
    MUST_SUCCEED(consume(RIGHT_PAREN, "Expect ')' after parameters."));

    MUST_SUCCEED(consume(LEFT_BRACE, "Expect '{' before function body."));
    BIND(body, statementList());

    return context.makeFunDecl(name, std::move(params), std::move(body));
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
    if (match(FOR)) return forStatement();
    if (match(IF)) return ifStatement();
    if (match(PRINT)) return printStatement();
    if (match(RET)) return returnStatement();
    if (match(WHILE)) return whileStatement();
    if (match(LEFT_BRACE)) return block();

    return expressionStatement();
}

// Desugaring into while.
std::optional<StatementIndex> Parser::forStatement()
{
    MUST_SUCCEED(consume(LEFT_PAREN, "Expect '(' after for."));

    std::optional<StatementIndex> init;
    if (!match(SEMICOLON))
    {
        // Semi is consumed/checked by init statement parsing.
        if (match(VAR))
        {
            BIND(vardecl, varDeclaration());
            init = vardecl;
        }
        else
        {
            BIND(exprStmt, expressionStatement());
            init = exprStmt;
        }
    }

    std::optional<ExpressionIndex> cond;
    if (!check(SEMICOLON))
    {
        BIND(c, expression());
        cond = c;
    }
    MUST_SUCCEED(consume(SEMICOLON, "Expect ';' after loop condition."));


    std::optional<ExpressionIndex> incr;
    if (!check(RIGHT_PAREN))
    {
        BIND(i, expression());
        incr = i;
    }
    MUST_SUCCEED(consume(RIGHT_PAREN, "Expect ')' after for caluses."));

    BIND(body, statement());

    if (incr)
        body = context.makeBlock({body, context.makeExprStmt(*incr)});

    // Empty condition is desugared into a synthesized true literal.
    if (!cond)
    {
        auto trueIdx = TokenList::getSyntheticTrueIdx();
        cond = context.makeLiteral(Index<Token>{trueIdx});
    }
    body = context.makeWhile(*cond, body);

    if (!init)
        return body;

    return context.makeBlock({*init, body});
}

std::optional<Index<IfStatement>> Parser::ifStatement()
{
    MUST_SUCCEED(consume(LEFT_PAREN, "Expect '(' after if."));
    BIND(condition, expression());
    MUST_SUCCEED(consume(RIGHT_PAREN, "Expect ')' after if condition."));

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

std::optional<Index<Return>> Parser::returnStatement()
{
    Index<Token> keyword = previous();
    std::optional<ExpressionIndex> value;
    if (!check(SEMICOLON))
    {
        BIND(arg, expression());
        value = arg;
    }

    consume(SEMICOLON, "Expect ';' after return value.");
    return context.makeReturn(keyword, value);
}

std::optional<Index<WhileStatement>> Parser::whileStatement()
{
    MUST_SUCCEED(consume(LEFT_PAREN, "Expect '(' after while."));
    BIND(condition, expression());
    MUST_SUCCEED(consume(RIGHT_PAREN, "Expect ')' after while condition."));

    BIND(body, statement());

    return context.makeWhile(condition, body);
}

std::optional<Index<Block>> Parser::block()
{
    BIND(statements, statementList());
    return context.makeBlock(std::move(statements));
}

std::optional<std::vector<StatementIndex>> Parser::statementList()
{
    std::vector<StatementIndex> statements;

    while(!check(RIGHT_BRACE) && !isAtEnd())
    {
        BIND(stmt, declaration());
        statements.push_back(stmt);
    }

    consume(RIGHT_BRACE, "Expect '}' after block.");

    return statements;
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
    BIND(expr, or_());

    if (match(EQUAL))
    {
        Index<Token> equals = previous();
        BIND(value, assignment());

        if (const auto* dRefId = get_if<Index<DeclRef>>(&expr))
        {
            // TODO: simplify this pattern.
            const auto* dRefNode = std::get<const DeclRef*>(context.getNode(*dRefId));
            return context.makeAssign(dRefNode->name, value);
        }

        error(equals, "Invalid assignment target");
        return std::nullopt;
    }

    return expr;
}

std::optional<ExpressionIndex> Parser::or_()
{
    BIND(expr, and_());

    while(match(OR))
    {
        Index<Token> op = previous();
        BIND(right, and_());
        expr = context.makeBinary(expr, op, right);
    }

    return expr;
}

std::optional<ExpressionIndex> Parser::and_()
{
    BIND(expr, equality());

    while(match(AND))
    {
        Index<Token> op = previous();
        BIND(right, equality());
        expr = context.makeBinary(expr, op, right);
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

    return call();
}

std::optional<ExpressionIndex> Parser::call()
{
    BIND(expr, primary());

    while (true)
    {
        if (match(LEFT_PAREN))
        {
            BIND(c, finishCall(previous(), expr));
            expr = c;
        }
        else
            break;
    }

    return expr;
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
        MUST_SUCCEED(consume(RIGHT_PAREN, "Expect ')' after expression"));
        Index<Token> end = previous();
        return context.makeGrouping(begin, expr, end);
    }

    error(peek(), "Unexpected token.");
    return std::nullopt;
}

std::optional<ExpressionIndex> Parser::finishCall(Index<Token> begin, ExpressionIndex callee)
{
    std::vector<ExpressionIndex> args;

    if (!check(RIGHT_PAREN))
    {
        do
        {
            if (args.size() >= 255)
            {
                error(peek(), "Can't have more than 255 arguments.");
                return std::nullopt;
            }

            BIND(arg, expression());
            args.push_back(arg);
        } while (match(COMMA));
    }

    BIND(end, consume(RIGHT_PAREN, "Expect ')' after arguments."));

    return context.makeCall(callee, begin, std::move(args), end);
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

void Parser::error(Index<Token> tIdx, std::string_view message) noexcept
{
    const Token& t = context.getToken(tIdx);
    if (t.type == END_OF_FILE)
    {
        diag.report(t.line, "at end of file", message);
    }
    else
    {
        diag.report(t.line, fmt::format("at '{}'", print(t)), message);
    }
}
