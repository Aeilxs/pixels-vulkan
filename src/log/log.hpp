#pragma once

#include <source_location>
#include <utility>

extern "C" {
#include <log.h>
}

// Keep rxi/log.c as an implementation detail of the C++ logging facade.
#undef log_trace
#undef log_debug
#undef log_info
#undef log_warn
#undef log_error
#undef log_fatal

namespace ps::log {
namespace color {

#if defined(LOG_USE_COLOR)
inline constexpr char reset[] = "\x1b[0m";
inline constexpr char gray[] = "\x1b[90m";
inline constexpr char cyan[] = "\x1b[36m";
inline constexpr char yellow[] = "\x1b[33m";
inline constexpr char magenta[] = "\x1b[35m";
#else
inline constexpr char reset[] = "";
inline constexpr char gray[] = "";
inline constexpr char cyan[] = "";
inline constexpr char yellow[] = "";
inline constexpr char magenta[] = "";
#endif

}  // namespace color

// Avoid anonymous namespace in a header: each translation unit
// including this file would get its own distinct write() function.
namespace detail {
struct Message {
    const char* format;
    std::source_location location;

    constexpr Message(const char* format, std::source_location location = std::source_location::current()) noexcept
        : format{format}, location{location} {
    }
};

template <typename... Args>
void write(int level, Message message, Args&&... args) {
    ::log_log(level, message.location.file_name(), static_cast<int>(message.location.line()), message.format, std::forward<Args>(args)...);
}
}  // namespace detail

inline void initialize() noexcept {
    ::log_set_level(APPLICATION_LOG_LEVEL);
}

inline const char* configuredLevelName() noexcept {
    return ::log_level_string(APPLICATION_LOG_LEVEL);
}

template <typename... Args>
void trace(detail::Message message, Args&&... args) {
    detail::write(LOG_TRACE, message, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(detail::Message message, Args&&... args) {
    detail::write(LOG_DEBUG, message, std::forward<Args>(args)...);
}

template <typename... Args>
void info(detail::Message message, Args&&... args) {
    detail::write(LOG_INFO, message, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(detail::Message message, Args&&... args) {
    detail::write(LOG_WARN, message, std::forward<Args>(args)...);
}

template <typename... Args>
void error(detail::Message message, Args&&... args) {
    detail::write(LOG_ERROR, message, std::forward<Args>(args)...);
}

template <typename... Args>
void fatal(detail::Message message, Args&&... args) {
    detail::write(LOG_FATAL, message, std::forward<Args>(args)...);
}
}  // namespace ps::log
