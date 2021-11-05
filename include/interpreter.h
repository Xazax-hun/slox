#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string_view>

bool runFile(std::string_view path);
bool runSource(std::string sourceText);
bool runPrompt();

class Context
{
public:
private:
    bool hadError;
};

#endif