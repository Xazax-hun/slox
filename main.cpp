#include <string_view>

#include <fmt/format.h>
#include <include/interpreter.h>

using namespace std::literals;

int main(int argc, const char* argv[])
{
    auto printHelp = [&]()
    {
        fmt::print("Usage: {} [script] [options]\n", argv[0]);
        fmt::print("options:\n");
        fmt::print("  --ast-dump\n");
        fmt::print("  --help\n");
    };

    const char *file = nullptr;
    bool dumpAst = false;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            // Process flags.
            if (argv[i] == "--ast-dump"sv)
            {
                dumpAst = true;
                continue;
            }
            if (argv[i] == "--help"sv)
            {
                printHelp();
                return EXIT_SUCCESS;
            }

            printHelp();
            return EXIT_FAILURE;
        }
        if (!file)
        {
            file = argv[i];
            continue;
        }

        printHelp();
        return EXIT_FAILURE;
    }

    if (file)
        return runFile(file, dumpAst) ? EXIT_SUCCESS : EXIT_FAILURE;

    return runPrompt(dumpAst) ? EXIT_SUCCESS : EXIT_FAILURE;
}