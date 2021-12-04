#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <sstream>

#include "include/eval.h"
#include "include/lexer.h"
#include "include/parser.h"

namespace
{

std::optional<std::string> evalCode(std::string sourceCode)
{
    std::stringstream output;
    DiagnosticEmitter emitter(output, output);
    Lexer lexer(std::move(sourceCode), emitter);
    auto maybeTokens = lexer.lexAll();
    if (!maybeTokens)
        return std::nullopt;

    Parser parser(emitter);
    parser.addTokens(std::move(*maybeTokens));
    auto maybeAst = parser.parse();
    if (!maybeAst)
        return std::nullopt;

    Interpreter interpreter(parser.getContext(), emitter);
    if (!interpreter.evaluate(*maybeAst))
        return std::nullopt;

    return std::move(output).str();
}

void checkOutputOfCode(std::string_view code, std::string_view expectedOutput)
{
    auto output = evalCode(std::string(code));
    EXPECT_TRUE(output.has_value());
    EXPECT_EQ(*output, expectedOutput);
}

TEST(Eval, BasicNodes)
{
    std::pair<std::string_view, std::string_view> checks[] =
    {
        // Comment.
        {"//print 1;", ""},

        // Expressions.
        {"print 1;", "1.0\n"},
        {"print -1;", "-1.0\n"},
        {"print true;", "true\n"},
        {"print false;", "false\n"},
        {"print !false;", "true\n"},
        {"print !true;", "false\n"},
        {"print \"hello\";", "hello\n"},
        {"print nil;", "nil\n"},
        {"print 2 + 3 * 4 - 6;", "8.0\n"},
        {"print (2 + 3) * (4 - 6);", "-10.0\n"},
        {"print 6 / 3;", "2.0\n"},
        {"print false or 5;", "5.0\n"},
        {"print true and nil;", "nil\n"},

        // Assignment.
        {"var a = 1; print a;", "1.0\n"},
        {"var a = 1; a = 2; print a;", "2.0\n"},
        {"var a; var b; a = b = 2; print a;", "2.0\n"},

        // Statements.
        {"if (true) print 1.0;", "1.0\n"},
        {"if (false) print 1.0;", ""},
        {"if (false) print 1.0; print 2.0;", "2.0\n"},
        {"if (false) { print 1.0; print 2.0; }", ""},
        {"if (false) nil; else print 1.0;", "1.0\n"},
        {"var a = 3; while (a > 0) { print a; a = a - 1; }", "3.0\n2.0\n1.0\n"},
        {"for(var a = 3; a > 0; a = a - 1) { print a; }", "3.0\n2.0\n1.0\n"},
        {"fun f() { return 5; } print f();", "5.0\n"},
        {"fun f() { print 5; } f();", "5.0\n"},
    };

    for (auto check : checks)
        checkOutputOfCode(check.first, check.second);

    // TODO: redo the tests in incremental mode.
}

TEST(Eval, RuntimeErrors)
{
}

TEST(Eval, Scoping)
{
    std::pair<std::string_view, std::string_view> checks[] =
    {
        // Blocks in sequence.
        {" {var a = 1; print a; } { var a = 2; print a;}", "1.0\n2.0\n"},

        // Shadowing.
        {"var a = 1; { var a = 2; print a; } print a;", "2.0\n1.0\n"},
        {"var a = 1; fun f(a){ print a; } f(2);", "2.0\n"},

        // Function can modify globals.
        {"var a = 1; fun f() { a = 2; } f(); print a;", "2.0\n"},

        // Closures.
        {"fun makeCounter() {"
         "  var i = 1;"
         "  fun counter() {"
         "    print i;"
         "    i = i + 1;"
         "  }"
         "  return counter;"
         "}"
         "var c = makeCounter();"
         "c(); c(); c(); c(); c();", "1.0\n2.0\n3.0\n4.0\n5.0\n"},

        // Closure is snapshot.
        {"var global = 1;"
         "{"
         "  fun printGlobal() {"
         "    print global;"
         "  }"
         "  printGlobal();"
         "  var global = 2;"
         "  printGlobal();"
         "}", "1.0\n1.0\n"},
    };

    for (auto check : checks)
        checkOutputOfCode(check.first, check.second);
}
} // anonymous namespace