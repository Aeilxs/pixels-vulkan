#pragma once

#include <cstdint>
#include <filesystem>
#include <ostream>
#include <stdexcept>

namespace ps::app::cli {

class Error final : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

struct AppOptions {
    bool help{false};
    std::filesystem::path image_path{};
    std::uint32_t gap{1};
};

void usage(const char* program_name, std::ostream& output_stream);

AppOptions parse(int argc, char* argv[]);

}  // namespace ps::app::cli
