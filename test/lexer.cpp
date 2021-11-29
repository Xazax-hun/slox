#include <gtest/gtest.h>

#include <algorithm>

#include "include/lexer.h"
#include "include/utils.h"

namespace
{
using enum TokenType;

auto lexString (std::string s, std::ostream& output)
{
    DiagnosticEmitter emitter(output, output);
    Lexer lexer(std::move(s), emitter);
    return lexer.lexAll();
}

TEST(Lexer, TestAllTokens)
{
    // Tokens with values.
    {
        std::stringstream output;
        auto token = lexString("identName", output)->front();
        EXPECT_EQ(IDENTIFIER, token.type);
        EXPECT_EQ("identName", std::get<std::string>(token.value));
        EXPECT_TRUE(output.str().empty());
    }
    {
        std::stringstream output;
        auto token = lexString("\"literal\"", output)->front();
        EXPECT_EQ(STRING, token.type);
        EXPECT_EQ("literal", std::get<std::string>(token.value));
        EXPECT_TRUE(output.str().empty());
    }
    {
        std::stringstream output;
        auto token = lexString("0.0", output)->front();
        EXPECT_EQ(NUMBER, token.type);
        EXPECT_EQ(0.0, std::get<double>(token.value));
        EXPECT_TRUE(output.str().empty());
    }

    // Keywords.
    {
        std::stringstream output;
        auto tokenList = lexString("and class else false fun for if nil or"
                                   " print return super this true var while", output).value();
        TokenType tokenTypes[] = {AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL,
                                  OR, PRINT, RET, SUPER, THIS, TRUE, VAR, WHILE,
                                  END_OF_FILE};
        EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                    [](Token t, TokenType type) { return t.type == type; }));
        EXPECT_TRUE(output.str().empty());
    }

    // Operators and separators.
    {
        std::stringstream output;
        auto tokenList = lexString("() {} ,.-+; / * ! != = == > >= < <=", output).value();
        TokenType tokenTypes[] = {
            LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, COMMA, DOT, MINUS, PLUS, SEMICOLON,
            SLASH, STAR, BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL, GREATER, GREATER_EQUAL,
            LESS, LESS_EQUAL, END_OF_FILE};
        EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                    [](Token t, TokenType type) { return t.type == type; }));
        EXPECT_TRUE(output.str().empty());
    }
}

TEST(Lexer, Comments)
{
    // No tokens in file.
    {
        std::stringstream output;
        auto tokenList = lexString("", output).value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);

        tokenList = lexString("\n", output).value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);

        tokenList = lexString("\r\t\n\r\t   \n", output).value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);

        tokenList = lexString("// A comment only.", output).value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);

        tokenList = lexString("// A comment only.\n", output).value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);

        tokenList = lexString("\n\t // A comment only.\n", output).value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);
        EXPECT_TRUE(output.str().empty());
    }

    // Tokens and comments.
    {
        std::stringstream output;
        auto tokenList = lexString("and // 0.4 ( } class \"\n else", output).value();
        TokenType tokenTypes[] = {AND, ELSE, END_OF_FILE};
        EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                    [](Token t, TokenType type) { return t.type == type; }));
        EXPECT_TRUE(output.str().empty());
    }
}

TEST(Lexer, LineNumbers)
{
    // Start from 1.
    {
        std::stringstream output;
        auto tokenList = lexString("", output).value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(1, tokenList.front().line);
        EXPECT_TRUE(output.str().empty());
    }

    // Count new lines.
    {
        std::stringstream output;
        auto tokenList = lexString("\nand\nor\n", output).value();
        EXPECT_EQ(3, tokenList.size());
        EXPECT_EQ(2, tokenList.front().line);
        EXPECT_EQ(4, tokenList.back().line);
        EXPECT_TRUE(output.str().empty());
    }

    // Count new lines in strings.
    {
        std::stringstream output;
        auto tokenList = lexString("\"fooo\nbaar\n\"\n", output).value();
        EXPECT_EQ(2, tokenList.size());
        EXPECT_EQ(4, tokenList.back().line);
        EXPECT_TRUE(output.str().empty());
    }
}

TEST(Lexer, Escaping)
{
    std::stringstream output;
    auto tokenList = lexString(R"("Hello\t\"world!\"\n")", output).value();
    EXPECT_EQ(2, tokenList.size());
    EXPECT_EQ("Hello\t\"world!\"\n", std::get<std::string>(tokenList.front().value));
    EXPECT_TRUE(output.str().empty());
}

TEST(Lexer, ErrorMessages)
{
    {
        std::stringstream output;
        auto maybeTokens = lexString("|", output);
        EXPECT_FALSE(maybeTokens.has_value());
        EXPECT_EQ("[line 1] Error : Unexpected token: '|'.\n", output.str());
    }

    {
        std::stringstream output;
        auto maybeTokens = lexString("\"\\|\"", output);
        EXPECT_FALSE(maybeTokens.has_value());
        EXPECT_EQ("[line 1] Error : Unknown escape sequence '\\|'.\n", output.str());
    }

    {
        std::stringstream output;
        auto maybeTokens = lexString("\"This is unterminated.", output);
        EXPECT_FALSE(maybeTokens.has_value());
        EXPECT_EQ("[line 1] Error : Unterminated string.\n", output.str());
    }
}

} // namespace
