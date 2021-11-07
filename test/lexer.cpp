#include "gtest/gtest.h"
#include "include/lexer.h"
#include <algorithm>
#include <iostream>

namespace {

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
        EXPECT_EQ(TokenType::IDENTIFIER, token.type);
        EXPECT_EQ("identName", std::get<std::string>(token.value));
    }
    {
        auto token = lexString("\"literal\"")->front();
        EXPECT_EQ(TokenType::STRING, token.type);
        EXPECT_EQ("literal", std::get<std::string>(token.value));
    }
    {
        auto token = lexString("0.0")->front();
        EXPECT_EQ(TokenType::NUMBER, token.type);
        EXPECT_EQ(0.0, std::get<double>(token.value));
    }

    // Keywords.
    {
        auto tokenList = lexString("and class else false fun for if nil or"
                                   " print return super this true var while").value();
        TokenType tokenTypes[] = {TokenType::AND, TokenType::CLASS, TokenType::ELSE, TokenType::FALSE,
                                  TokenType::FUN, TokenType::FOR, TokenType::IF, TokenType::NIL,
                                  TokenType::OR, TokenType::PRINT, TokenType::RETURN, TokenType::SUPER,
                                  TokenType::THIS, TokenType::TRUE, TokenType::VAR, TokenType::WHILE,
                                  TokenType::END_OF_FILE};
        EXPECT_TRUE(std::equal(tokenList.begin(), tokenList.end(), std::begin(tokenTypes), std::end(tokenTypes),
                    [](Token t, TokenType type) { return t.type == type; }));
    }

    // Operators and separators.
    {
        auto tokenList = lexString("() {} ,.-+; / * ! != = == > >= < <=").value();
        TokenType tokenTypes[] = {
            TokenType::LEFT_PAREN, TokenType::RIGHT_PAREN, TokenType::LEFT_BRACE, TokenType::RIGHT_BRACE,
            TokenType::COMMA, TokenType::DOT, TokenType::MINUS, TokenType::PLUS, TokenType::SEMICOLON,
            TokenType::SLASH, TokenType::STAR, TokenType::BANG, TokenType::BANG_EQUAL,
            TokenType::EQUAL, TokenType::EQUAL_EQUAL, TokenType::GREATER, TokenType::GREATER_EQUAL,
            TokenType::LESS, TokenType::LESS_EQUAL, TokenType::END_OF_FILE};
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
        EXPECT_EQ(TokenType::END_OF_FILE, tokenList.front().type);

        tokenList = lexString("\n").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(TokenType::END_OF_FILE, tokenList.front().type);

        tokenList = lexString("\r\t\n\r\t   \n").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(TokenType::END_OF_FILE, tokenList.front().type);

        tokenList = lexString("// A comment only.").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(TokenType::END_OF_FILE, tokenList.front().type);

        tokenList = lexString("// A comment only.\n").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(TokenType::END_OF_FILE, tokenList.front().type);

        tokenList = lexString("\n\t // A comment only.\n").value();
        EXPECT_EQ(1, tokenList.size());
        EXPECT_EQ(TokenType::END_OF_FILE, tokenList.front().type);
    }

    // Tokens and comments.
    {
        auto tokenList = lexString("and // 0.4 ( } class \"\n else").value();
        TokenType tokenTypes[] = {TokenType::AND, TokenType::ELSE, TokenType::END_OF_FILE};
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

} // namespace