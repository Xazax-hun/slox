#include <include/lexer.h>
#include <include/utils.h>
#include <fmt/format.h>

#include <ctype.h>
#include <sstream>

std::string print(Token t)
{
    switch (t.type)
    {
    case TokenType::IDENTIFIER:
        return std::get<std::string>(t.value);

    case TokenType::STRING:
        return "\"" + std::get<std::string>(t.value) + "\"";
    case TokenType::NUMBER:
        return std::to_string(std::get<double>(t.value));
    
    default:
        return std::string(tokenTypeToSourceName(t.type));
    }
}

std::optional<std::vector<Token>> Lexer::lexAll()
{
    std::vector<Token> result;
    while (!isAtEnd())
    {
        if (auto maybeToken = lex(); maybeToken.has_value())
            result.push_back(*maybeToken);
        else if (hasError)
            return std::nullopt;
    }

    result.emplace_back(TokenType::END_OF_FILE, line);
    return result;
}

std::optional<Token> Lexer::lex()
{
    while (true)
    {
        start = current;
        char c = advance();
        switch (c)
        {
        //  Unambiguous single characters tokens.
        case '(': return Token(TokenType::LEFT_PAREN, line);
        case ')': return Token(TokenType::RIGHT_PAREN, line);
        case '{': return Token(TokenType::LEFT_BRACE, line);
        case '}': return Token(TokenType::RIGHT_BRACE, line);
        case ',': return Token(TokenType::COMMA, line);
        case '.': return Token(TokenType::DOT, line);
        case '-': return Token(TokenType::MINUS, line);
        case '+': return Token(TokenType::PLUS, line);
        case ';': return Token(TokenType::SEMICOLON, line);
        case '*': return Token(TokenType::STAR, line);

        //  Single or double character tokens.
        case '!':
            return match('=') ? Token(TokenType::BANG_EQUAL, line) : 
                                Token(TokenType::BANG, line);
        case '=':
            return match('=') ? Token(TokenType::EQUAL_EQUAL, line) : 
                                Token(TokenType::EQUAL, line);
        case '<':
            return match('=') ? Token(TokenType::LESS_EQUAL, line) : 
                                Token(TokenType::LESS, line);
        case '>':
            return match('=') ? Token(TokenType::GREATER_EQUAL, line) : 
                                Token(TokenType::GREATER, line);

        // Longer tokens.
        case '/':
            if (match('/'))
            {
                // Skip to end of line for comment.
                while (peek() != '\n' && !isAtEnd())
                    advance();
                break;
            }
            
            return Token(TokenType::SLASH, line);

        // Whitespace.
        case '\n':
            line++;
            [[fallthrough]];
        case ' ':
        case '\r':
        case '\t':
            break;

        case '"':
            return lexString();
            
        
        default:
            if (isdigit(c))
                return lexNumber();

            if (isalpha(c))
                return lexIdentifier();

            error(line, fmt::format("Unexpected token: '{}'.", source.substr(start, current - start)));
            hasError = true;
            return std::nullopt;
        }

        if (isAtEnd())
            return std::nullopt;
    }
}

std::optional<Token> Lexer::lexString()
{
    // TODO: Add support for escaping.
    while (peek() != '"' && !isAtEnd())
    {
        if (peek() == '\n')
            line++;
        advance();
    }

    if (isAtEnd()) {
        error(line, "Unterminated string.");
        return std::nullopt;
    }

    // Skip closing ".
    advance();
    // Trim surrounding quotes.
    return Token(TokenType::STRING, line, source.substr(start + 1, current - start - 2));
}

std::optional<Token> Lexer::lexNumber()
{
    while (isdigit(peek()))
        advance();

    // Look for a fractional part.
    if (peek() == '.' && isdigit(peekNext()))
    {
        // Consume the "."
        advance();

        while (isdigit(peek()))
            advance();
    }

    std::stringstream convert;
    convert << source.substr(start, current - start);
    double value;
    convert >> value;

    return Token(TokenType::NUMBER, line, value);
}

std::optional<Token> Lexer::lexIdentifier()
{
    while (isalnum(peek()))
        advance();

    auto text = source.substr(start, current - start);
    if (auto it = keywords.find(text); it != keywords.end())
        return Token(it->second, line);

    return Token(TokenType::IDENTIFIER, line, text);
}

bool Lexer::match(char expected)
{
    if (isAtEnd())
        return false;
    if (source[current] != expected)
        return false;

    current++;
    return true;
}

char Lexer::peek()
{
    if (isAtEnd())
        return false;
    return source[current];
}

char Lexer::peekNext()
{
    if (static_cast<unsigned>(current + 1) >= source.length())
        return false;
    return source[current + 1];
}


// TODO: use constexpr std::string_view tokenTypeToSourceName(TokenType);
const std::unordered_map<std::string_view, TokenType> Lexer::keywords = {
    {tokenTypeToSourceName(TokenType::AND),    TokenType::AND},
    {tokenTypeToSourceName(TokenType::CLASS),  TokenType::CLASS},
    {tokenTypeToSourceName(TokenType::ELSE),   TokenType::ELSE},
    {tokenTypeToSourceName(TokenType::FALSE),  TokenType::FALSE},
    {tokenTypeToSourceName(TokenType::FOR),    TokenType::FOR},
    {tokenTypeToSourceName(TokenType::FUN),    TokenType::FUN},
    {tokenTypeToSourceName(TokenType::IF),     TokenType::IF},
    {tokenTypeToSourceName(TokenType::NIL),    TokenType::NIL},
    {tokenTypeToSourceName(TokenType::OR),     TokenType::OR},
    {tokenTypeToSourceName(TokenType::PRINT),  TokenType::PRINT},
    {tokenTypeToSourceName(TokenType::RETURN), TokenType::RETURN},
    {tokenTypeToSourceName(TokenType::SUPER),  TokenType::SUPER},
    {tokenTypeToSourceName(TokenType::THIS),   TokenType::THIS},
    {tokenTypeToSourceName(TokenType::TRUE),   TokenType::TRUE},
    {tokenTypeToSourceName(TokenType::VAR),    TokenType::VAR},
    {tokenTypeToSourceName(TokenType::WHILE),  TokenType::WHILE}
};
