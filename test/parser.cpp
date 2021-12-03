#include <gtest/gtest.h>

#include "include/lexer.h"
#include "include/parser.h"
#include "include/utils.h"

#include <string_view>
#include <utility>
#include <sstream>

namespace
{
struct ParseResult
{
    Index<Unit> unit;
    std::string dumped;
    ASTContext ctxt;
};

std::optional<ParseResult> parseText(std::string_view sourceText, std::ostream& out)
{
    DiagnosticEmitter emitter(out, out);
    Lexer lexer{std::string(sourceText), emitter};
    auto maybeTokens = lexer.lexAll();
    if (!maybeTokens)
        return std::nullopt;

    Parser parser(emitter);
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

    auto expectAstForSource = [](std::string_view sourceText, std::string_view astDump)
    {
        std::stringstream output;
        auto result = parseText(sourceText, output);
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(astDump, result->dumped);
        EXPECT_TRUE(output.str().empty());
    };

    std::pair<std::string_view, std::string_view> checks[] =
    {
        // Comments are ignored.
        {"// Foobar", "(unit)"},

        // Literals.
        {"print 5;", "(unit (print 5.000000))"},
        {"print \"hello\";", "(unit (print \"hello\"))"},
        {"print true;", "(unit (print true))"},
        {"print false;", "(unit (print false))"},
        {"print nil;", "(unit (print nil))"},

        // Unary operators.
        {"-a; !b;", "(unit (exprStmt (- a)) (exprStmt (! b)))"},

        // Binary operators.
        // Precedence.
        {"a + 2 * 3 - 4;", "(unit (exprStmt (- (+ a (* 2.000000 3.000000)) 4.000000)))"},
        // Grouping.
        {"(a + 2) / (3 - 4);", "(unit (exprStmt (/ (group (+ a 2.000000)) (group (- 3.000000 4.000000)))))"},
        // Logical.
        {"a < b or b > c and a == c;", "(unit (exprStmt (or (< a b) (and (> b c) (== a c)))))"},
        {"a <= b or b >= c and a != c;", "(unit (exprStmt (or (<= a b) (and (>= b c) (!= a c)))))"},
        // Assignment.
        {"a = b = c + 1;", "(unit (exprStmt (= a (= b (+ c 1.000000)))))"},

        // Function call.
        {"f(); g(a, b+1, (c));", "(unit (exprStmt (call f)) (exprStmt (call g a (+ b 1.000000) (group c))))"},

        // Statements.
        // Expression statements are tested with expressions.
        // Variable declaration.
        {"var a = 1;", "(unit (var a 1.000000))"},
        {"var b;", "(unit (var b <NULL>))"},
        // Function declaration and return statement.
        {"fun bar() { return 5; }", "(unit (fun bar (body (return 5.000000))))"},
        {"fun bar(a, b, c) { print a + b * c; }", "(unit (fun bar a b c (body (print (+ a (* b c))))))"},
        // Blocks.
        {"var a = 1; { var a = 2; {} }", "(unit (var a 1.000000) (block (var a 2.000000) (block)))"},
        // If.
        {"if (a) { a = a + 1; }", "(unit (if a (block (exprStmt (= a (+ a 1.000000)))) <NULL>))"},
        {"if (a) { a = a + 1; } else a = a - 1;", "(unit (if a (block (exprStmt (= a (+ a 1.000000)))) (exprStmt (= a (- a 1.000000)))))"},
        // While.
        {"while (a) { a = a + 1; print a; }", "(unit (while a (block (exprStmt (= a (+ a 1.000000))) (print a))))"},
        {"while (a > 0) f(a);", "(unit (while (> a 0.000000) (exprStmt (call f a))))"},
        // For.
        // TODO: add cases where certain elements are missing.
        {"for(var a = 1; a < 10; a = a + 1) print a;", "(unit (block (var a 1.000000) (while (< a 10.000000) (block (print a) (exprStmt (= a (+ a 1.000000)))))))"},
    };

    for (auto [source, astDump] : checks)
        expectAstForSource(source, astDump);
}

TEST(Parser, ErrorMessages)
{
    auto expectErrorForSource = [](std::string_view sourceText, std::string_view errorText)
    {
        std::stringstream output;
        parseText(sourceText, output);
        EXPECT_EQ(errorText , output.str());
    };

    std::pair<std::string_view, std::string_view> checks[] =
    {
        // Function declarations.
        {"fun 1", "[line 1] Error at '1.000000': Expect function name.\n"},
        {"fun name other", "[line 1] Error at 'other': Expect '(' after function name.\n"},
        {"fun name (1", "[line 1] Error at '1.000000': Expect parameter name.\n"},
        {"fun name (a", "[line 1] Error at end of file: Expect ')' after parameters.\n"},
        {"fun name (a)", "[line 1] Error at end of file: Expect '{' before function body.\n"},

        // Variable declaration.
        {"var 1", "[line 1] Error at '1.000000': Expect variable name.\n"},
        {"var a", "[line 1] Error at end of file: Expect ';' after variable declaration.\n"},
        {"var a = 1", "[line 1] Error at end of file: Expect ';' after variable declaration.\n"},

        // For statement.
        // TODO: optional loop conditon.
        {"for", "[line 1] Error at end of file: Expect '(' after for.\n"},
        {"for(x; x > 0", "[line 1] Error at end of file: Expect ';' after loop condition.\n"},
        {"for(x; x > 0; x = x - 1", "[line 1] Error at end of file: Expect ')' after for caluses.\n"},

        // If statement.
        {"if x", "[line 1] Error at 'x': Expect '(' after if.\n"},
        {"if (x", "[line 1] Error at end of file: Expect ')' after if condition.\n"},

        // Expression statement.
        {"x", "[line 1] Error at end of file: Expect ';' after value.\n"},

        // Return statement.
        {"return x", "[line 1] Error at end of file: Expect ';' after return value.\n"},

        // While statement.
        {"while", "[line 1] Error at end of file: Expect '(' after while.\n"},
        {"while (x", "[line 1] Error at end of file: Expect ')' after while condition.\n"},

        // Block statement.
        {"{ x; ", "[line 1] Error at end of file: Expect '}' after block.\n"},

        // Assignment.
        {"1 = x; ", "[line 1] Error at '=': Invalid assignment target\n"},

        // Grouping.
        {"(x or y; ", "[line 1] Error at ';': Expect ')' after expression\n"},

        // Call.
        {"f(a, b, c; ", "[line 1] Error at ';': Expect ')' after arguments.\n"},
    };

    for (auto [source, error] : checks)
        expectErrorForSource(source, error);
}

TEST(Parser, ErrorRecovery)
{
    // TODO
}

} // anonymous namespace
