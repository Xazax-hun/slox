#ifndef UTILS_H
#define UTILS_H

#include <string>

void error(int line, std::string message);
void report(int line, std::string where, std::string message);

#endif