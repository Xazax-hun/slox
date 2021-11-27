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

bool runSource(std::string sourceText, bool dumpAst)
{
    Lexer lexer(sourceText);
    auto maybeTokens = lexer.lexAll();
    if (!maybeTokens)
        return false;

    Parser parser;
    parser.addTokens(std::move(*maybeTokens));
    auto maybeAst = parser.parse();
    if (!maybeAst)
        return false;

    if (dumpAst)
    {
        ASTPrinter printer(parser.getContext());
        fmt::print("{}\n", printer.print(*maybeAst));
    }

    Interpreter interpreter(parser.getContext());
    if (!interpreter.evaluate(*maybeAst))
        return false;

    return true;
}

namespace
{
constexpr std::string_view prompt{"> "};
constexpr std::string_view promptCont{".."};
constexpr std::string_view indent{"  "};

std::string getPrompt(int indent)
{
    if (indent == 0)
        return std::string(::prompt);
    
    std::string promptText(::promptCont);
    while (indent-- > 0)
        promptText += ::indent;

    return promptText;
}
} // anonymous namespace

bool runPrompt(bool dumpAst)
{
    std::string line;
    int indent = 0;
    std::string indentText;

    Parser parser;
    Interpreter interpreter(parser.getContext());

    while (true)
    {
        std::string line;
        {
            char *lineRaw = readline(::getPrompt(indent).data());
            if (!lineRaw)
                break;
            add_history(lineRaw);
            line = lineRaw;
        }

        Lexer lexer(line);
        auto maybeTokens = lexer.lexAll();
        if (!maybeTokens)
            return false;

        parser.addTokens(std::move(*maybeTokens));

        // We only want to parse complete declarations/statements.
        // If the brackets are unbalanced, we wait for more input
        // before actually doing some parsing.
        indent += lexer.getBracketBalance();
        if (indent > 0)
        {
            continue;
        }

        auto maybeAst = parser.parse();
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
