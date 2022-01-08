#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <sstream>

#include "include/eval.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/interpreter.h"

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
    interpreter.evaluate(*maybeAst);

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
        {"print 1.0;", "1\n"},
        {"print -1;", "-1\n"},
        {"print true;", "true\n"},
        {"print false;", "false\n"},
        {"print !false;", "true\n"},
        {"print !true;", "false\n"},
        {"print \"hello\";", "hello\n"},
        {"print nil;", "nil\n"},
        {"print 2 + 3 * 4 - 6;", "8\n"},
        {"print (2 + 3) * (4 - 6);", "-10\n"},
        {"print 6 / 3;", "2\n"},
        {"print false or 5;", "5\n"},
        {"print true and nil;", "nil\n"},
        {R"(print "Hello " + "world!";)", "Hello world!\n"},

        // Assignment.
        {"var a = 1; print a;", "1\n"},
        {"var a = 1; a = 2; print a;", "2\n"},
        {"var a; var b; a = b = 2; print a;", "2\n"},

        // Statements.
        {"if (true) print 1;", "1\n"},
        {"if (false) print 1;", ""},
        {"if (nil) print 1; print 2;", "2\n"},
        {"if (false) { print 1; print 2; }", ""},
        {"if (false) nil; else print 1;", "1\n"},
        {"var a = 3; while (a > 0) { print a; a = a - 1; }", "3\n2\n1\n"},
        {"for(var a = 3; a > 0; a = a - 1) { print a; }", "3\n2\n1\n"},
        {"fun f() { return 5; } print f();", "5\n"},
        {"fun f() { print 5; } f();", "5\n"},
        // TODO: add break and test for conditionless for.
    };

    for (auto check : checks)
        checkOutputOfCode(check.first, check.second);

    // Retest everything in incremental mode,
    // i.e., parsing line by line.
    std::string allCode;
    std::string allExpectedOutput;
    for (auto check : checks)
    {
        allCode += check.first;
        allCode += '\n';
        allExpectedOutput += check.second;
    }
    std::stringstream input(allCode);
    std::stringstream output;
    runPrompt(input, output, output);
    EXPECT_EQ(std::move(output).str(), allExpectedOutput);
}

TEST(Eval, StaticErrors)
{
    std::pair<std::string_view, std::string_view> checks[] =
    {
        {"return;", "[line 1] Error : Can't return from top level code\n"},
        {"fun f() { var a = 1; var a = 1; }", "[line 1] Error : Already a variable with name 'a' in this scope.\n"},
        {"fun f() { var a = a; }", "[line 1] Error : Can't read local variable in its own initializer.\n"},
    };

    for (auto check : checks)
        checkOutputOfCode(check.first, check.second);
}

TEST(Eval, RuntimeErrors)
{
    std::pair<std::string_view, std::string_view> checks[] =
    {
        {"5 / \"hello\";", "[line 1] Error : Operand must evaluate to a number.\n"},
        {"5 * \"hello\";", "[line 1] Error : Operand must evaluate to a number.\n"},
        {"\"hello\" - 5;", "[line 1] Error : Operand must evaluate to a number.\n"},
        {"\"hello\" + 5;", "[line 1] Error : Operands' type mismatch.\n"},
        {"nil + nil;", "[line 1] Error : Operands with unsupported type.\n"},
        {"\"hello\" > 5;", "[line 1] Error : Operand must evaluate to a number.\n"},
        {"\"hello\" < 5;", "[line 1] Error : Operand must evaluate to a number.\n"},
        {"\"hello\" <= 5;", "[line 1] Error : Operand must evaluate to a number.\n"},
        {"\"hello\" >= 5;", "[line 1] Error : Operand must evaluate to a number.\n"},
        {"a;", "[line 1] Error : Undefined variable: 'a'.\n"},
        {"a = 1;", "[line 1] Error : Undefined variable: 'a'.\n"},
        {"4(1, 2, 3);", "[line 1] Error : Can only call functions and classes.\n"},
    };

    for (auto check : checks)
        checkOutputOfCode(check.first, check.second);
}

TEST(Eval, Scoping)
{
    std::pair<std::string_view, std::string_view> checks[] =
    {
        // Blocks in sequence.
        {" {var a = 1; print a; } { var a = 2; print a;}", "1\n2\n"},

        // Shadowing.
        {"var a = 1; { var a = 2; print a; } print a;", "2\n1\n"},
        {"var a = 1; fun f(a){ print a; } f(2);", "2\n"},

        // Function can modify globals.
        {"var a = 1; fun f() { a = 2; } f(); print a;", "2\n"},

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
         "c(); c(); c(); c(); c();", "1\n2\n3\n4\n5\n"},

        // Closure is snapshot.
        {"var global = 1;"
         "{"
         "  fun printGlobal() {"
         "    print global;"
         "  }"
         "  printGlobal();"
         "  var global = 2;"
         "  printGlobal();"
         "}", "1\n1\n"},
    };

    for (auto check : checks)
        checkOutputOfCode(check.first, check.second);
}
} // anonymous namespace