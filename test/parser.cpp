#include "gtest/gtest.h"

#include "include/lexer.h"
#include "include/parser.h"

#include <string_view>
#include <utility>

namespace
{
using enum TokenType;

struct ParseResult
{
    Index<Unit> unit;
    std::string dumped;
    ASTContext ctxt;
};

std::optional<ParseResult> parseText(std::string_view sourceText)
{
    Lexer lexer{std::string(sourceText)};
    auto maybeTokens = lexer.lexAll();
    if (!maybeTokens)
        return std::nullopt;

    Parser parser;
    parser.addTokens(std::move(*maybeTokens));
    auto maybeAst = parser.parse();
    if (!maybeAst)
        return std::nullopt;

    ASTPrinter printer(parser.getContext());
    
    return ParseResult{*maybeAst, printer.print(*maybeAst), parser.getContext()};
}

// TODO: add small matcher framework to help verify the shape of
// of the AST in memory without dumping.
TEST(Parser, AllNodesParsed)
{
    // Comment is ignored.
    {
        auto result = parseText("// Foobar");
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ("(unit)", result->dumped);
    }

    // Literals.
    {
        {
            auto result = parseText("print 5;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (print 5.000000))", result->dumped);
        }

        {
            auto result = parseText("print \"hello\";");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (print \"hello\"))", result->dumped);
        }

        {
            auto result = parseText("print true;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (print true))", result->dumped);
        }

        {
            auto result = parseText("print false;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (print false))", result->dumped);
        }

        {
            auto result = parseText("print nil;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (print nil))", result->dumped);
        }
    }

    // Unary operators.
    {
        auto result = parseText("-a; !b;");
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ("(unit (exprStmt (- a)) (exprStmt (! b)))", result->dumped);
    }

    // Binary operators.
    {
        // Precedence.
        {
            auto result = parseText("a + 2 * 3 - 4;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (exprStmt (- (+ a (* 2.000000 3.000000)) 4.000000)))", result->dumped);
        }

        // Grouping.
        {
            auto result = parseText("(a + 2) / (3 - 4);");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (exprStmt (/ (group (+ a 2.000000)) (group (- 3.000000 4.000000)))))", result->dumped);
        }

        // Logical.
        {
            auto result = parseText("a < b or b > c and a == c;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (exprStmt (or (< a b) (and (> b c) (== a c)))))", result->dumped);
        }

        {
            auto result = parseText("a <= b or b >= c and a != c;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (exprStmt (or (<= a b) (and (>= b c) (!= a c)))))", result->dumped);
        }

        // Assignment.
        {
            auto result = parseText("a = b = c + 1;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (exprStmt (= a (= b (+ c 1.000000)))))", result->dumped);
        }
    }

    // Function call.
    {
        auto result = parseText("f(); g(a, b+1, (c));");
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ("(unit (exprStmt (call f)) (exprStmt (call g a (+ b 1.000000) (group c))))", result->dumped);
    }

    // Statements.
    {
        // Expression statements are tested with expressions.

        // Variable declaration.
        {
            auto result = parseText("var a = 1;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (var a 1.000000))", result->dumped);
        }

        {
            auto result = parseText("var b;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (var b <NULL>))", result->dumped);
        }

        // Function declaration and return statement.
        {
            auto result = parseText("fun bar() { return 5; }");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (fun bar (body (return 5.000000))))", result->dumped);
        }

        {
            auto result = parseText("fun bar(a, b, c) { print a + b * c; }");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (fun bar a b c (body (print (+ a (* b c))))))", result->dumped);
        }

        // Blocks.
        {
            auto result = parseText("var a = 1; { var a = 2; {} }");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (var a 1.000000) (block (var a 2.000000) (block)))", result->dumped);
        }

        // If.
        {
            auto result = parseText("if (a) { a = a + 1; }");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (if a (block (exprStmt (= a (+ a 1.000000)))) <NULL>))", result->dumped);
        }

        {
            auto result = parseText("if (a) { a = a + 1; } else a = a - 1;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (if a (block (exprStmt (= a (+ a 1.000000)))) (exprStmt (= a (- a 1.000000)))))", result->dumped);
        }

        // While.
        {
            auto result = parseText("while (a) { a = a + 1; print a; }");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (while a (block (exprStmt (= a (+ a 1.000000))) (print a))))", result->dumped);
        }
        
        {
            auto result = parseText("while (a > 0) f(a);");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (while (> a 0.000000) (exprStmt (call f a))))", result->dumped);
        }

        // For.
        // TODO: add cases where certain elements are missing.
        {
            auto result = parseText("for(var a = 1; a < 10; a = a + 1) print a;");
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ("(unit (block (var a 1.000000) (while (< a 10.000000) (block (print a) (exprStmt (= a (+ a 1.000000)))))))", result->dumped);
        }
    }
}

TEST(Parser, ErrorMessages)
{
    // TODO: add dependency injection so it is possible to extract errors.
}

TEST(Parser, ErrorRecovery)
{
    // TODO
}

} // anonymous namespace