#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>

// use enum class to avoid name conflicts with bison
enum class LogLevel : unsigned int { DEBUG = 0, INFO, WARNING, ERROR };
struct LocationInfo {
    LocationInfo(std::string file, int line, const char *func) : file_(file), line_(line), func_(func) {}
    ~LocationInfo() = default;

    std::string file_;
    int line_;
    const char *func_;
};
class LogStream;
class LogWriter;

// RELEASE 模式下不打印 LOG，加快编译时间(毕竟 RELEASE 模式的 LOG 我们一般也拿不到)
class LogWriter {
  public:
    LogWriter(LocationInfo location, LogLevel loglevel) : location_(location), log_level_(loglevel) {
#ifdef ENABLE_LOG
        char *logv = std::getenv("LOGV");
        if (logv) {
            std::string string_logv = logv;
            env_log_level = std::stoi(logv);
        } else {
            env_log_level = 0;  // default output all logs
        }
#else
#warning disable log in non debug mode
        env_log_level = static_cast<unsigned int>(LogLevel::ERROR) + 1;
#endif
    };

    void operator<(const LogStream &stream);

  private:
    void output_log(const std::ostringstream &g);
    LocationInfo location_;
    LogLevel log_level_;
    unsigned int env_log_level;
};

class LogStream {
  public:
    LogStream() : sstream_(new std::stringstream()) {}
    ~LogStream() = default;

    template <typename T>
    LogStream &operator<<(const T &val) noexcept {
        (*sstream_) << val;
        return *this;
    }

    friend class LogWriter;

  private:
    std::shared_ptr<std::stringstream> sstream_;
};

std::string level2string(LogLevel level);
std::string get_short_name(const char *file_path);

#define __FILESHORTNAME__ get_short_name(__FILE__)
#define LOG_IF(level) LogWriter(LocationInfo(__FILESHORTNAME__, __LINE__, __FUNCTION__), level) < LogStream()
#define LOG(level) LOG_##level
#define LOG_DEBUG LOG_IF(LogLevel::DEBUG)
#define LOG_INFO LOG_IF(LogLevel::INFO)
#define LOG_WARNING LOG_IF(LogLevel::WARNING)
#define LOG_ERROR LOG_IF(LogLevel::ERROR)

#endif
