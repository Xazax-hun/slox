#include <include/lexer.h>
#include <include/utils.h>
#include <fmt/format.h>

#include <ctype.h>
#include <sstream>

using enum TokenType;

std::string print(const Token& t)
{
    switch (t.type)
    {
    case IDENTIFIER:
        return std::get<std::string>(t.value);

    case STRING:
        return "\"" + std::get<std::string>(t.value) + "\"";
    case NUMBER:
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

    result.emplace_back(END_OF_FILE, line);
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
        case '(': return Token(LEFT_PAREN, line);
        case ')': return Token(RIGHT_PAREN, line);
        case '{': return Token(LEFT_BRACE, line);
        case '}': return Token(RIGHT_BRACE, line);
        case ',': return Token(COMMA, line);
        case '.': return Token(DOT, line);
        case '-': return Token(MINUS, line);
        case '+': return Token(PLUS, line);
        case ';': return Token(SEMICOLON, line);
        case '*': return Token(STAR, line);

        //  Single or double character tokens.
        case '!':
            return match('=') ? Token(BANG_EQUAL, line) : 
                                Token(BANG, line);
        case '=':
            return match('=') ? Token(EQUAL_EQUAL, line) : 
                                Token(EQUAL, line);
        case '<':
            return match('=') ? Token(LESS_EQUAL, line) : 
                                Token(LESS, line);
        case '>':
            return match('=') ? Token(GREATER_EQUAL, line) : 
                                Token(GREATER, line);

        // Longer tokens.
        case '/':
            if (match('/'))
            {
                // Skip to end of line for comment.
                while (peek() != '\n' && !isAtEnd())
                    advance();
                break;
            }
            
            return Token(SLASH, line);

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
    return Token(STRING, line, source.substr(start + 1, current - start - 2));
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

    return Token(NUMBER, line, value);
}

std::optional<Token> Lexer::lexIdentifier()
{
    while (isalnum(peek()))
        advance();

    auto text = source.substr(start, current - start);
    if (auto it = keywords.find(text); it != keywords.end())
        return Token(it->second, line);

    return Token(IDENTIFIER, line, text);
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

char Lexer::peek() const
{
    if (isAtEnd())
        return false;
    return source[current];
}

char Lexer::peekNext() const
{
    if (static_cast<unsigned>(current + 1) >= source.length())
        return false;
    return source[current + 1];
}


// TODO: use constexpr std::string_view tokenTypeToSourceName(TokenType);
const std::unordered_map<std::string_view, TokenType> Lexer::keywords = {
    {tokenTypeToSourceName(AND),    AND},
    {tokenTypeToSourceName(CLASS),  CLASS},
    {tokenTypeToSourceName(ELSE),   ELSE},
    {tokenTypeToSourceName(FALSE),  FALSE},
    {tokenTypeToSourceName(FOR),    FOR},
    {tokenTypeToSourceName(FUN),    FUN},
    {tokenTypeToSourceName(IF),     IF},
    {tokenTypeToSourceName(NIL),    NIL},
    {tokenTypeToSourceName(OR),     OR},
    {tokenTypeToSourceName(PRINT),  PRINT},
    {tokenTypeToSourceName(RET),    RET},
    {tokenTypeToSourceName(SUPER),  SUPER},
    {tokenTypeToSourceName(THIS),   THIS},
    {tokenTypeToSourceName(TRUE),   TRUE},
    {tokenTypeToSourceName(VAR),    VAR},
    {tokenTypeToSourceName(WHILE),  WHILE}
};
