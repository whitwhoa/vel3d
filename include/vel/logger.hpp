#pragma once

#include <atomic>
#include <spdlog/spdlog.h>

namespace vel
{
    // Lazy accessor: returns the "vel3d" logger if registered, else nullptr.
    // Caches the raw pointer after the first successful lookup (fast hot path).
    inline spdlog::logger* get_vel3d_logger()
    {
        static std::atomic<spdlog::logger*> cache{ nullptr };

        if (auto* p = cache.load(std::memory_order_acquire))
            return p;

        if (auto sp = spdlog::get("vel3d"))
        {
            auto* p = sp.get();
            cache.store(p, std::memory_order_release);
            return p;
        }

        return nullptr; // strict no-op if not registered
    }
}

// -------- Unconditional macros (no-op if "vel3d" isn't registered) --------
#define VEL3D_LOG_TRACE(...)    do { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_TRACE(lg, __VA_ARGS__); } while (0)
#define VEL3D_LOG_DEBUG(...)    do { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_DEBUG(lg, __VA_ARGS__); } while (0)
#define VEL3D_LOG_INFO(...)     do { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_INFO (lg, __VA_ARGS__); } while (0)
#define VEL3D_LOG_WARN(...)     do { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_WARN (lg, __VA_ARGS__); } while (0)
#define VEL3D_LOG_ERROR(...)    do { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_ERROR(lg, __VA_ARGS__); } while (0)
#define VEL3D_LOG_CRITICAL(...) do { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_CRITICAL(lg, __VA_ARGS__); } while (0)

// -------- Conditional (_IF) variants --------
#define VEL3D_LOG_TRACE_IF(cond, ...)    do { if (cond) { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_TRACE(lg, __VA_ARGS__); } } while (0)
#define VEL3D_LOG_DEBUG_IF(cond, ...)    do { if (cond) { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_DEBUG(lg, __VA_ARGS__); } } while (0)
#define VEL3D_LOG_INFO_IF(cond,  ...)    do { if (cond) { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_INFO (lg, __VA_ARGS__); } } while (0)
#define VEL3D_LOG_WARN_IF(cond,  ...)    do { if (cond) { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_WARN (lg, __VA_ARGS__); } } while (0)
#define VEL3D_LOG_ERROR_IF(cond, ...)    do { if (cond) { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_ERROR(lg, __VA_ARGS__); } } while (0)
#define VEL3D_LOG_CRITICAL_IF(cond, ...) do { if (cond) { auto* lg = vel::get_vel3d_logger(); if (lg) SPDLOG_LOGGER_CRITICAL(lg, __VA_ARGS__); } } while (0)

// -------- (force flush all registered loggers) --------
#define VEL3D_LOG_FLUSH() do { spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& l){ l->flush(); }); } while (0)