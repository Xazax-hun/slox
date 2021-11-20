#include <include/interpreter.h>

#include <fstream>
#include <sstream>

#include <readline/readline.h>
#include <readline/history.h>
#include <fmt/format.h>

#include <include/lexer.h>
#include <include/ast.h>
#include <include/parser.h>
#include <include/eval.h>

constexpr std::string_view prompt{"> "};

bool runFile(std::string_view path, bool dumpAst)
{
    std::ifstream file(path.data());
    if (!file) 
        return false;

    // TODO: Do we might not need to store
    //       the full source text in memory.
    std::stringstream buffer;
    buffer << file.rdbuf();
    return runSource(std::move(buffer).str(), dumpAst);
}

bool runPrompt(bool dumpAst)
{
    std::string line;
    // TODO: more sophisticated solution for persisting env across lines.
    Environment env;
    Parser parser;
    Interpreter interpreter(parser.getContext(), env);
    while (true)
    {
        std::string line;
        {
            char *lineRaw = readline(::prompt.data());
            if (!lineRaw)
                break;
            add_history(lineRaw);
            line = lineRaw;
        }

        Lexer lexer(line);
        auto maybeTokens = lexer.lexAll();
        if (!maybeTokens)
            return false;

        auto maybeAst = parser.parse(std::move(*maybeTokens));
        if (!maybeAst)
            return false;

        if (dumpAst)
        {
            ASTPrinter printer(parser.getContext());
            fmt::print("{}\n",printer.print(*maybeAst));
        }

        if (!interpreter.evaluate(*maybeAst))
            return false;
    }
    return true;
}

bool runSource(std::string sourceText, bool dumpAst)
{
    Lexer lexer(sourceText);
    auto maybeTokens = lexer.lexAll();
    if (!maybeTokens)
        return false;

    Parser parser;
    auto maybeAst = parser.parse(std::move(*maybeTokens));
    if (!maybeAst)
        return false;

    if (dumpAst)
    {
        ASTPrinter printer(parser.getContext());
        fmt::print("{}\n",printer.print(*maybeAst));
    }

    Interpreter interpreter(parser.getContext());
    if (!interpreter.evaluate(*maybeAst))
        return false;

    return true;
}
