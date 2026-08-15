#include "app/app.hpp"
#include "app/cli.hpp"

#include <exception>
#include <iostream>

namespace cli = ps::app::cli;
using ps::app::App;

int run(int argc, char* argv[]) {
    const cli::AppOptions options = cli::parse(argc, argv);

    if (options.help) {
        cli::usage(argv[0], std::cout);
        return 0;
    }

    App app{options};
    app.run();

    return 0;
}

int main(int argc, char* argv[]) {
    try {
        return run(argc, argv);
    } catch (const cli::Error& exception) {
        cli::usage(argv[0], std::cerr);
        std::cerr << exception.what() << '\n';
        return 1;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
