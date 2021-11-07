#include <include/utils.h>
#include <iostream>
#include <fmt/format.h>


void error(int line, std::string message)
{
    report(line, "", message);
}

void report(int line, std::string where, std::string message)
{
    fmt::print(stderr, "[line {}] Error {}: {}\n", line, where, message);
}