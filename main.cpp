#include <fmt/format.h>
#include <include/interpreter.h>

int main(int argc, const char* argv[])
{
    if (argc >= 3)
    {
        fmt::print("Usage: {} [script]\n", argv[0]);
        return EXIT_SUCCESS;
    }
    else if (argc == 2)
    {
        runFile(argv[1]);
    }
    else
    {
        runPrompt();
    }

    return EXIT_SUCCESS;
}