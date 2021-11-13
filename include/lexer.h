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
    switch(type)
    {
        case TokenType::LEFT_PAREN: return "(";
        case TokenType::RIGHT_PAREN: return ")";
        case TokenType::LEFT_BRACE: return "{";
        case TokenType::RIGHT_BRACE: return "}";
        case TokenType::COMMA: return ",";
        case TokenType::DOT: return ".";
        case TokenType::MINUS: return "-";
        case TokenType::PLUS: return "+";
        case TokenType::SEMICOLON: return ";";
        case TokenType::SLASH: return "/";
        case TokenType::STAR: return "*";
        case TokenType::BANG: return "!";
        case TokenType::BANG_EQUAL: return "!=";
        case TokenType::EQUAL: return "=";
        case TokenType::EQUAL_EQUAL: return "==";
        case TokenType::GREATER: return ">";
        case TokenType::GREATER_EQUAL: return ">=";
        case TokenType::LESS: return "<";
        case TokenType::LESS_EQUAL: return "<=";
        case TokenType::IDENTIFIER: return "IDENT";
        case TokenType::STRING: return "STRING";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::AND: return "and";
        case TokenType::CLASS: return "class";
        case TokenType::ELSE: return "else";
        case TokenType::FALSE: return "false";
        case TokenType::FUN: return "fun";
        case TokenType::FOR: return "for";
        case TokenType::IF: return "if";
        case TokenType::NIL: return "nil";
        case TokenType::OR: return "or";
        case TokenType::PRINT: return "print";
        case TokenType::RET: return "return";
        case TokenType::SUPER: return "super";
        case TokenType::THIS: return "this";
        case TokenType::TRUE: return "true";
        case TokenType::VAR: return "var";
        case TokenType::WHILE: return "while";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
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
    bool hasError = false; // TODO: get rid of this.

    static const std::unordered_map<std::string_view, TokenType> keywords;
};

#endif