#include <include/utils.h>
#include <fmt/format.h>


void error(int line, std::string message) noexcept
{
    report(line, "", std::move(message));
}

void report(int line, std::string where, std::string message) noexcept
{
    fmt::print(stderr, "[line {}] Error {}: {}\n", line,
        std::move(where), std::move(message));
}