#include <include/lexer.h>
#include <include/utils.h>

#include <fmt/format.h>

#include <cstdlib>
#include <unordered_map>

using enum TokenType;

std::string print(const Token& t) noexcept
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

TokenList::TokenList() noexcept
{
    // True token to support synthesizing while statements from
    // for expressions with empty condition.
    tokens.emplace_back(TRUE, -1);
    firstNonSynthetic = tokens.size();
}

void TokenList::mergeTokensFrom(TokenList&& other) noexcept
{
    // Get rid if the now incorrect end of file token.
    if (!tokens.empty())
        tokens.pop_back();

    auto firstTokenFromSourceIt = other.tokens.begin();
    
    // We only need to add the synthetic tokens once.
    if (!tokens.empty())
        firstTokenFromSourceIt += firstNonSynthetic;

    tokens.insert(tokens.end(), std::make_move_iterator(firstTokenFromSourceIt),
                  std::make_move_iterator(other.tokens.end()));
}

std::optional<TokenList> Lexer::lexAll() noexcept
{
    TokenList result;

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

std::optional<Token> Lexer::lex() noexcept
{
    while (true)
    {
        start = current;
        char c = advance();
        switch (c)
        {
        //  Unambiguous single characters tokens.
        case '(':
            ++bracketBalance;
            return Token(LEFT_PAREN, line);
        case ')':
            --bracketBalance;
            return Token(RIGHT_PAREN, line);
        case '{':
            ++bracketBalance;
            return Token(LEFT_BRACE, line);
        case '}':
            --bracketBalance;
            return Token(RIGHT_BRACE, line);

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
            // TODO: support /* */ style comments.
            
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

            diag.error(line, fmt::format("Unexpected token: '{}'.", source.substr(start, current - start)));
            hasError = true;
            return std::nullopt;
        }

        if (isAtEnd())
            return std::nullopt;
    }
}

std::optional<Token> Lexer::lexString() noexcept
{
    std::string content;
    bool escaping = false;
    while (!isAtEnd())
    {
        if (escaping)
        {
            switch (peek())
            {
            case '\\':
            case '"':
                content += peek();
                break;

            case 'n':
                content += '\n';
                break;
            case 't':
                content += '\t';
                break;
            
            default:
                diag.error(line, fmt::format("Unknown escape sequence '\\{}'.", peek()));
                hasError = true;
                return std::nullopt;
            }
            escaping = false;
            advance();
            continue;
        }

        if (peek() == '"')
            break;

        if (peek() == '\\')
        {
            escaping = true;
            advance();
            continue;
        }
        
        if (peek() == '\n')
            line++;
        content += advance();
    }

    if (isAtEnd()) {
        diag.error(line, "Unterminated string.");
        hasError = true;
        return std::nullopt;
    }

    // Skip closing ".
    advance();
    // Trim surrounding quotes.
    return Token(STRING, line, content);
}

std::optional<Token> Lexer::lexNumber() noexcept
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

    double value = strtod(source.substr(start, current - start).c_str(), nullptr);

    return Token(NUMBER, line, value);
}

namespace {
const std::unordered_map<std::string_view, TokenType> keywords = {
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
} // anonymous namespace

std::optional<Token> Lexer::lexIdentifier() noexcept
{
    while (isalnum(peek()))
        advance();

    auto text = source.substr(start, current - start);
    if (auto it = keywords.find(text); it != keywords.end())
        return Token(it->second, line);

    return Token(IDENTIFIER, line, text);
}

bool Lexer::match(char expected) noexcept
{
    if (isAtEnd())
        return false;
    if (source[current] != expected)
        return false;

    current++;
    return true;
}

char Lexer::peek() const noexcept
{
    if (isAtEnd())
        return '\0';
    return source[current];
}

char Lexer::peekNext() const noexcept
{
    if (static_cast<unsigned>(current + 1) >= source.length())
        return '\0';
    return source[current + 1];
}
