#pragma once

#include <iostream>
#include <string_view>

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace berry {

struct logger {
inline static std::shared_ptr<spdlog::logger> consoleSink = nullptr;
inline static std::shared_ptr<spdlog::logger> fileSink = nullptr;
inline static std::shared_ptr<spdlog::logger> mainLogger = nullptr;

static void init() noexcept
{
    try {
        consoleSink = spdlog::stdout_color_mt<spdlog::async_factory>("console_logger");
#ifdef NDEBUG
        consoleSink->sinks()[0]->set_level(spdlog::level::info);
#else
        consoleSink->sinks()[0]->set_level(spdlog::level::debug);
#endif
        fileSink = spdlog::basic_logger_mt<spdlog::async_factory>("file_logger", "logs/log.txt", true);
        fileSink->sinks()[0]->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks { consoleSink->sinks()[0], fileSink->sinks()[0] };
        mainLogger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
        mainLogger->set_pattern("[%T][%^%=7t%$][%^%=7l%$] %v");
        mainLogger->set_level(spdlog::level::trace);

        spdlog::flush_every(std::chrono::seconds(3));
    } catch (spdlog::spdlog_ex const& ex) {
        std::cout << "File log init failed: " << ex.what() << std::endl;
    }
}

static void deinit() noexcept
{
    spdlog::shutdown();
}
};

namespace log {

template<typename... Args>
inline void trace(fmt::format_string<Args...> fmt, Args&&... args) noexcept
{
    logger::mainLogger->trace(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void debug(fmt::format_string<Args...> fmt, Args&&... args) noexcept
{
    logger::mainLogger->debug(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void info(fmt::format_string<Args...> fmt, Args&&... args) noexcept
{
    logger::mainLogger->info(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void warn(fmt::format_string<Args...> fmt, Args&&... args) noexcept
{
    logger::mainLogger->warn(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void error(fmt::format_string<Args...> fmt, Args&&... args) noexcept
{
    logger::mainLogger->error(fmt, std::forward<Args>(args)...);
    logger::fileSink->flush();
}

inline void info(std::string_view sv) noexcept
{
    logger::mainLogger->info(sv);
}
inline void debug(std::string_view sv) noexcept
{
    logger::mainLogger->debug(sv);
}
inline void error(std::string_view sv) noexcept
{
    logger::mainLogger->error(sv);
}

inline void timer(std::string_view msg, double time)
{
    trace("{:>6.2f}s  {}", time, msg);
}
}

}
