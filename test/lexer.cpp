#include "gtest/gtest.h"
#include "include/lexer.h"
#include <algorithm>

namespace
{
using enum TokenType;

auto lexString (std::string s)
{
    Lexer lexer(std::move(s));
    return lexer.lexAll();
}

TEST(Lexer, TestAllTokens)
{
    // Tokens with values.
    {
        auto token = lexString("identName")->front();
        EXPECT_EQ(IDENTIFIER, token.type);
        EXPECT_EQ("identName", std::get<std::string>(token.value));
    }
    {
        auto token = lexString("\"literal\"")->front();
        EXPECT_EQ(STRING, token.type);
        EXPECT_EQ("literal", std::get<std::string>(token.value));
    }
    {
        auto token = lexString("0.0")->front();
        EXPECT_EQ(NUMBER, token.type);
        EXPECT_EQ(0.0, std::get<double>(token.value));
    }

    // Keywords.
    {
        auto tokenList = lexString("and class else false fun for if nil or"
                                   " print return super this true var while").value();
        TokenType tokenTypes[] = {AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL,
                                  OR, PRINT, RET, SUPER, THIS, TRUE, VAR, WHILE,
                                  END_OF_FILE};
        EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                    [](Token t, TokenType type) { return t.type == type; }));
    }

    // Operators and separators.
    {
        auto tokenList = lexString("() {} ,.-+; / * ! != = == > >= < <=").value();
        TokenType tokenTypes[] = {
            LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, COMMA, DOT, MINUS, PLUS, SEMICOLON,
            SLASH, STAR, BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL, GREATER, GREATER_EQUAL,
            LESS, LESS_EQUAL, END_OF_FILE};
        EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                    [](Token t, TokenType type) { return t.type == type; }));
    }
}

TEST(Lexer, Comments)
{
    // No tokens in file.
    {
        auto tokenList = lexString("").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);

        tokenList = lexString("\n").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);

        tokenList = lexString("\r\t\n\r\t   \n").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);

        tokenList = lexString("// A comment only.").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);

        tokenList = lexString("// A comment only.\n").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);

        tokenList = lexString("\n\t // A comment only.\n").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(END_OF_FILE, tokenList.front().type);
    }

    // Tokens and comments.
    {
        auto tokenList = lexString("and // 0.4 ( } class \"\n else").value();
        TokenType tokenTypes[] = {AND, ELSE, END_OF_FILE};
        EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                    [](Token t, TokenType type) { return t.type == type; }));
    }
}

TEST(Lexer, LineNumbers)
{
    // Start from 1.
    {
        auto tokenList = lexString("").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(1, tokenList.front().line);
    }

    // Count new lines.
    {
        auto tokenList = lexString("\nand\nor\n").value();
        EXPECT_EQ(3, tokenList.size());
        EXPECT_EQ(2, tokenList.front().line);
        EXPECT_EQ(4, tokenList.back().line);
    }

    // Count new lines in strings.
    {
        auto tokenList = lexString("\"fooo\nbaar\n\"\n").value();
        EXPECT_EQ(2, tokenList.size());
        EXPECT_EQ(4, tokenList.back().line);
    }
}

// TODO: add tests for ambiguous lexing, e.g.: ===

} // namespace