#ifndef LEXER_H
#define LEXER_H

#include <variant>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <unordered_map>
#include <cassert>

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

    Token(TokenType type, int line, Value value = {}) :
        type(type), line(line), value(std::move(value)) {}
};

std::string print(const Token&);

class Lexer
{
public:
    Lexer(std::string source) : source(std::move(source)) {}

    std::optional<std::vector<Token>> lexAll();

    int getBracketBalance() { return bracketBalance; }

private:
    std::optional<Token> lex();
    std::optional<Token> lexString();
    std::optional<Token> lexNumber();
    std::optional<Token> lexIdentifier();
    bool isAtEnd() const { return static_cast<unsigned>(current) >= source.length(); }
    char advance() { return source[current++]; }
    char peek() const;
    char peekNext() const;
    bool match(char expected);

    std::string source;
    int start = 0;
    int current = 0;
    int line = 1;
    int bracketBalance = 0;
    bool hasError = false; // TODO: get rid of this.

    static const std::unordered_map<std::string_view, TokenType> keywords;
};

#endif