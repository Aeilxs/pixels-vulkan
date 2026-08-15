#include "app/cli.hpp"

#include <charconv>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace ps::app::cli {
namespace {

std::string_view next(int argc, char* argv[], int& index, std::string_view option) {
    if (index + 1 >= argc) {
        throw Error("Missing value for " + std::string(option));
    }

    ++index;
    return argv[index];
}

std::uint32_t parseGap(std::string_view value) {
    std::uint32_t gap{};

    const char* begin = value.data();
    const char* end = value.data() + value.size();
    const std::from_chars_result result = std::from_chars(begin, end, gap);

    if (result.ec != std::errc{} || result.ptr != end || gap == 0) {
        throw Error("Gap must be a positive integer: " + std::string(value));
    }

    return gap;
}

void validate(const AppOptions& options) {
    if (options.help) {
        return;
    }

    if (options.image_path.empty()) {
        throw Error("Image path is required. Use -i or --image.");
    }

    if (!std::filesystem::exists(options.image_path)) {
        throw std::runtime_error("Image file does not exist: " + options.image_path.string());
    }
}

}  // namespace

AppOptions parse(int argc, char* argv[]) {
    AppOptions options{};

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};

        if (arg == "-h" || arg == "--help") {
            options.help = true;
            continue;
        }

        if (arg == "-i" || arg == "--image") {
            options.image_path = next(argc, argv, i, arg);
            continue;
        }

        if (arg == "-g" || arg == "--gap") {
            const std::string_view value = next(argc, argv, i, arg);
            options.gap = parseGap(value);
            continue;
        }

        throw Error("Unknown option: " + std::string(arg));
    }

    validate(options);
    return options;
}

void usage(const char* arg_0, std::ostream& output_stream) {
    output_stream << R"(
▛▀▖▗       ▜  ▞▀▖▐            
▙▄▘▄ ▚▗▘▞▀▖▐  ▚▄ ▜▀ ▞▀▖▙▀▖▛▚▀▖
▌  ▐ ▗▚ ▛▀ ▐  ▖ ▌▐ ▖▌ ▌▌  ▌▐ ▌
▘  ▀▘▘ ▘▝▀▘ ▘ ▝▀  ▀ ▝▀ ▘  ▘▝ ▘   

Usage: )" << arg_0 << R"( [options]

Options:
  -h, --help          Show this help message and exit
  -i, --image <path>  Specify the path to the image file
  -g, --gap <value>   Specify the gap value (positive integer)
)" << '\n';
}

}  // namespace ps::app::cli
