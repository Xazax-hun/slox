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
        // Expressions.
        {"//print 1;", ""},
        {"print 1;", "1.0\n"},
    };

    for (auto check : checks)
    {
        checkOutputOfCode(check.first, check.second);
    }
}

TEST(Eval, RuntimeErrors)
{
}

TEST(Eval, Scoping)
{
    // TODO
}
} // anonymous namespace