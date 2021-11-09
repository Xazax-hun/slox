#ifndef UTILS_H
#define UTILS_H

#include <string>

// Create ad-hoc visitors with lambdas when using std::visit for variants.
template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

void error(int line, std::string message);
void report(int line, std::string where, std::string message);

#endif