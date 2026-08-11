#pragma once

namespace ps::platform {

/// @brief Owns the process-wide SDL video subsystem lifetime.
///
/// Construction initializes SDL video support and destruction shuts SDL down.
/// The context is intentionally neither copyable nor movable so that a single
/// object has an unambiguous responsibility for the SDL lifetime.
class SdlContext final {
   public:
    /// @brief Initializes the SDL video subsystem.
    /// @throws std::runtime_error If SDL initialization fails.
    SdlContext();

    /// @brief Shuts down all initialized SDL subsystems.
    ~SdlContext();

    SdlContext(const SdlContext&) = delete;
    SdlContext& operator=(const SdlContext&) = delete;

    SdlContext(SdlContext&&) = delete;
    SdlContext& operator=(SdlContext&&) = delete;
};

}  // namespace ps::platform
