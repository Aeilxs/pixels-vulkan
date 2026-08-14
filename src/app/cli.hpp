#pragma once

#include <cstdint>
#include <filesystem>
#include <ostream>
#include <stdexcept>

namespace ps::app::cli {

/// @brief Signals invalid command-line usage that should be accompanied by the usage text.
class Error final : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

struct AppOptions {
    bool help{false};
    std::filesystem::path image_path{};
    std::uint32_t gap{1};
};

/// @brief Prints the usage information for the application to the specified output stream.
/// @param program_name argv[0].
/// @param output_stream The output stream to which the usage information will be printed.
void usage(const char* program_name, std::ostream& output_stream);

/// @brief Parses the command line arguments and returns the resulting application options.
/// @param argc The number of command line arguments.
/// @param argv The array of command line arguments.
/// @return The parsed application options.
AppOptions parse(int argc, char* argv[]);

}  // namespace ps::app::cli
