#include <include/utils.h>
#include <fmt/format.h>


void error(int line, std::string message)
{
    report(line, "", std::move(message));
}

void report(int line, std::string where, std::string message)
{
    fmt::print(stderr, "[line {}] Error {}: {}\n", line,
        std::move(where), std::move(message));
}