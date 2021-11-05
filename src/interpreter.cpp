#include <include/interpreter.h>

#include <fstream>
#include <sstream>
#include <iostream>

#include <include/lexer.h>

constexpr std::string_view prompt{"> "};

bool runFile(std::string_view path)
{
    std::ifstream file(path.data());
    if (!file) 
        return false;

    // TODO: Do we might not need to store
    //       the full source text in memory.
    std::stringstream buffer;
    buffer << file.rdbuf();
    return runSource(std::move(buffer).str());
}

bool runPrompt()
{
    std::string line;
    while (true)
    {
        std::cout << ::prompt;
        if (!std::getline(std::cin, line))
            break;
        if (!runSource(std::move(line)))
            return false;
    }
    return true;
}

bool runSource(std::string sourceText)
{
    Lexer lexer(sourceText);
    auto maybeTokens = lexer.lexAll();
    if (!maybeTokens)
        return false;
    for (auto token : *maybeTokens)
    {
        std::cout << (int)token.type << " ";
    }
    return true;
}