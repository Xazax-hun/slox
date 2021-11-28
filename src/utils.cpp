#include <include/utils.h>
#include <fmt/format.h>


void DiagnosticEmitter::error(int line, std::string message) const noexcept
{
    report(line, "", std::move(message));
}

void DiagnosticEmitter::report(int line, std::string where, std::string message) const noexcept
{
    err << fmt::format("[line {}] Error {}: {}\n", line, std::move(where), std::move(message));
}