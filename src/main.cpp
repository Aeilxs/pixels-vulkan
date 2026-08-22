#include "app/app.hpp"
#include "app/cli.hpp"
#include "app/diagnostics.hpp"
#include "log/log.hpp"

#include <exception>
#include <iostream>

namespace cli = ps::app::cli;
namespace diagnostics = ps::app::diagnostics;
namespace log = ps::log;
using ps::app::App;

namespace {
int run(int argc, char* argv[]) {
    const cli::AppOptions options = cli::parse(argc, argv);
    if (options.help) {
        cli::usage(argv[0], std::cout);
        return 0;
    }

    log::initialize();
    diagnostics::logStartup(options);

    App app{options};
    app.run();

    return 0;
}
}  // namespace

int main(int argc, char* argv[]) {
    try {
        return run(argc, argv);
    } catch (const cli::Error& exception) {
        cli::usage(argv[0], std::cerr);
        ps::log::fatal("CLI Error: %s", exception.what());
        return 1;
    } catch (const std::exception& exception) {
        ps::log::fatal("Error: %s", exception.what());
        return 1;
    }
}
