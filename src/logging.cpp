#include "logging.hpp"

void LogWriter::operator<(const LogStream &stream) {
    std::ostringstream msg;
    msg << stream.sstream_->rdbuf();
    output_log(msg);
}

void LogWriter::output_log(const std::ostringstream &msg) {
    if (static_cast<unsigned int>(log_level_) >= env_log_level) {
        std::cout << "[" << level2string(log_level_) << "] "
                  << "(" << location_.file_ << ":" << location_.line_ << "L, " << location_.func_ << ") " << msg.str()
                  << std::endl;
        // 在打印完 LOG 的 ERROR 后应该用自定义退出码退出
        // if (log_level_ == LogLevel::ERROR)
        //     exit(1);
    }
    //     //  他看上去没有副作用，表现起来也没有副作用，但是删掉了就是会导致有些样例编译失败，这难道真的是玄学吗？
    //     level2string(log_level_);
}

std::string level2string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "\033[93;1mDEBUG\033[0m";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "\033[93;1mWARNING\033[0m";
        case LogLevel::ERROR:
            return "\033[31;1mERROR\033[0m";
    }
    return "ERROR";
}

std::string get_short_name(const char *file_path) {
    std::string short_file_path = file_path;
    int index = short_file_path.find_last_of('/');

    return short_file_path.substr(index + 1);
}
