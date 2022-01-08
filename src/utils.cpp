#include <include/utils.h>
#include <fmt/format.h>


void DiagnosticEmitter::error(int line, std::string_view message) const noexcept
{
    report(line, "", message);
}

void DiagnosticEmitter::report(int line, std::string_view where, std::string_view message) const noexcept
{
    err << fmt::format("[line {}] Error {}: {}\n", line, where, message);
}