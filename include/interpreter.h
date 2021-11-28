#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string_view>
#include <string>
#include <iosfwd>

bool runFile(std::string_view path, bool dumpAst = false);
bool runFile(std::string_view path, std::ostream& out, std::ostream& err, bool dumpAst = false);

bool runSource(std::string sourceText, bool dumpAst = false);
bool runSource(std::string sourceText, std::ostream& out, std::ostream& err, bool dumpAst = false);

bool runPrompt(bool dumpAst = false);
bool runPrompt(std::istream& in, std::ostream& out, std::ostream& err, bool dumpAst = false);

#endif