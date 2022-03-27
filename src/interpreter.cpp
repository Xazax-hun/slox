#include <include/interpreter.h>

#include <fstream>
#include <sstream>
#include <iostream>

#include <readline/readline.h>
#include <readline/history.h>
#include <fmt/format.h>

#include <include/lexer.h>
#include <include/ast.h>
#include <include/parser.h>
#include <include/eval.h>

bool runFile(std::string_view path, bool dumpAst)
{
    return runFile(path, std::cout, std::cerr, dumpAst);
}

bool runFile(std::string_view path, std::ostream& out, std::ostream& err, bool dumpAst)
{
    std::ifstream file(path.data());
    if (!file) 
        return false;

    // TODO: Do we might not need to store
    //       the full source text in memory.
    std::stringstream buffer;
    buffer << file.rdbuf();
    return runSource(std::move(buffer).str(), out, err, dumpAst);
}

bool runSource(std::string sourceText, bool dumpAst)
{
    return runSource(std::move(sourceText), std::cout, std::cerr, dumpAst);
}

bool runSource(std::string sourceText, std::ostream& out, std::ostream& err, bool dumpAst)
{
    DiagnosticEmitter emitter(out, err);
    Lexer lexer(std::move(sourceText), emitter);
    auto maybeTokens = lexer.lexAll();
    if (!maybeTokens)
        return false;

    Parser parser(emitter);
    parser.addTokens(std::move(*maybeTokens));
    auto maybeAst = parser.parse();
    if (!maybeAst)
        return false;

    if (dumpAst)
    {
        ASTPrinter printer(parser.getContext());
        fmt::print("{}\n", printer.print(*maybeAst));
    }

    Interpreter interpreter(parser.getContext(), emitter);
    return interpreter.evaluate(*maybeAst);
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
    return runPrompt(std::cin, std::cout, std::cerr, dumpAst);
}

bool runPrompt(std::istream& in, std::ostream& out, std::ostream& err, bool dumpAst)
{
    std::string line;
    int indent = 0;

    DiagnosticEmitter emitter(out, err);
    Parser parser(emitter);
    Interpreter interpreter(parser.getContext(), emitter);

    while (true)
    {
        if (&in == &std::cin)
        {
            char *lineRaw = readline(::getPrompt(indent).data());
            if (!lineRaw)
                break;
            add_history(lineRaw);
            line = lineRaw;
        }
        else
        {
            if (!std::getline(in, line))
                break;
        }

        Lexer lexer(line, emitter);
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
