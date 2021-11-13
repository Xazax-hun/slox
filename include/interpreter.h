#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string_view>
#include <string>

bool runFile(std::string_view path, bool dumpAst = false);
bool runSource(std::string sourceText, bool dumpAst = false);
bool runPrompt(bool dumpAst = false);

#endif