#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iosfwd>

// Create ad-hoc visitors with lambdas when using std::visit for variants.
template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

class DiagnosticEmitter
{
public:
    DiagnosticEmitter(std::ostream& out, std::ostream& err) noexcept
        : out(out), err(err) {}

    void error(int line, std::string message) const noexcept;
    void report(int line, std::string where, std::string message) const noexcept;

    std::ostream& getOutput() const { return out; }
private:
    std::ostream& out;
    std::ostream& err;
};

#endif