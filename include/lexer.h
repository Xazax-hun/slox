#ifndef LEXER_H
#define LEXER_H

#include <variant>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cassert>

#include <include/utils.h>

enum class TokenType : unsigned char
{
    // Single-character tokens.
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

    // One or two character tokens.
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    // Literals.
    IDENTIFIER, STRING, NUMBER,

    // Keywords.
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
    PRINT, RET, SUPER, THIS, TRUE, VAR, WHILE,

    END_OF_FILE
};

// LEFT_PAREN => "(", ...
constexpr std::string_view tokenTypeToSourceName(TokenType type)
{
    using enum TokenType;
    switch(type)
    {
        case LEFT_PAREN: return "(";
        case RIGHT_PAREN: return ")";
        case LEFT_BRACE: return "{";
        case RIGHT_BRACE: return "}";
        case COMMA: return ",";
        case DOT: return ".";
        case MINUS: return "-";
        case PLUS: return "+";
        case SEMICOLON: return ";";
        case SLASH: return "/";
        case STAR: return "*";
        case BANG: return "!";
        case BANG_EQUAL: return "!=";
        case EQUAL: return "=";
        case EQUAL_EQUAL: return "==";
        case GREATER: return ">";
        case GREATER_EQUAL: return ">=";
        case LESS: return "<";
        case LESS_EQUAL: return "<=";
        case IDENTIFIER: return "IDENT";
        case STRING: return "STRING";
        case NUMBER: return "NUMBER";
        case AND: return "and";
        case CLASS: return "class";
        case ELSE: return "else";
        case FALSE: return "false";
        case FUN: return "fun";
        case FOR: return "for";
        case IF: return "if";
        case NIL: return "nil";
        case OR: return "or";
        case PRINT: return "print";
        case RET: return "return";
        case SUPER: return "super";
        case THIS: return "this";
        case TRUE: return "true";
        case VAR: return "var";
        case WHILE: return "while";
        case END_OF_FILE: return "END_OF_FILE";
    }
    assert(false && "Unhandled token type");
    return "";
}

struct Token
{
    TokenType type;

    // TODO: add better location info:
    //       location should be an index into
    //       a table that has line number,
    //       column number and file path.
    unsigned line; 

    // The value of string, number literals.
    // The name of identifiers.
    // TODO: can we just reference the source text
    //       for string literals and identifier names?
    using Value = std::variant<std::string, double>;
    Value value;

    Token(TokenType type, int line, Value value = {}) noexcept :
        type(type), line(line), value(std::move(value)) {}
};

std::string print(const Token&) noexcept;

class Lexer
{
public:
    Lexer(std::string source, const DiagnosticEmitter& diag) noexcept
        : source(std::move(source)), diag(diag) {}

    std::optional<std::vector<Token>> lexAll() noexcept;

    int getBracketBalance() const noexcept { return bracketBalance; }

private:
    std::optional<Token> lex() noexcept;
    std::optional<Token> lexString() noexcept;
    std::optional<Token> lexNumber() noexcept;
    std::optional<Token> lexIdentifier() noexcept;
    bool isAtEnd() const noexcept { return static_cast<unsigned>(current) >= source.length(); }
    char advance() noexcept { return source[current++]; }
    char peek() const noexcept;
    char peekNext() const noexcept;
    bool match(char expected) noexcept;

    std::string source;
    const DiagnosticEmitter& diag;
    int start = 0;
    int current = 0;
    int line = 1;
    int bracketBalance = 0;
    bool hasError = false; // TODO: get rid of this.
};

#endif