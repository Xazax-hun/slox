#ifndef LEXER_H
#define LEXER_H

#include <variant>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <unordered_map>

enum class TokenType
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
    PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

    END_OF_FILE
};

// LEFT_PAREN => "(", ...
constexpr std::string_view tokenTypeToSourceName(TokenType);

struct Token
{
    TokenType type;

    // TODO: add better location info:
    //       location should be an index into
    //       a table that has line number,
    //       column number and file path.
    int line; 

    // The value of string, number literals.
    // The name of identifiers.
    using Value = std::variant<std::string, double>;
    Value value;

    Token(TokenType type, int line, Value value = {}) :
        type(type), line(line), value(std::move(value)) {}
};

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
    char peek();
    char peekNext();
    bool match(char expected);

    std::string source;
    int start = 0;
    int current = 0;
    int line = 1;

    static const std::unordered_map<std::string_view, TokenType> keywords;
};

#endif