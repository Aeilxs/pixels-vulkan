#include "app/app.hpp"
#include "app/cli.hpp"
#include "build/build_info.hpp"
#include "log/log.hpp"

#include <exception>
#include <iostream>

namespace build = ps::build;
namespace cli = ps::app::cli;
namespace log = ps::log;
using ps::app::App;

namespace {
int run(int argc, char* argv[]) {
    const cli::AppOptions options = cli::parse(argc, argv);
    if (options.help) {
        cli::usage(argv[0], std::cout);
        return 0;
    }

    log::init();
    log::info(
        "%s v%s [%s]\nBranch: %s\nCommit: @%s%s\nCompiler: %s",
        build::name,
        build::version,
        build::configuration,
        build::gitBranch,
        build::gitHash,
        build::gitDirty ? " (dirty)" : "",
        build::compiler
    );

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
        std::cerr << exception.what() << '\n';
        return 1;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
