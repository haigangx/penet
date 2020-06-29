#include "logger.h"

const char *levelStrs_[Logger::LogLevel::LALL + 1] = {
    "FATAL", "ERROR", "USER", "WARN", "INFO", "DEBUG", "TRACE", "ALL"
};

Logger::Logger()
{}

Logger::~Logger()
{}

Logger &Logger::getLogger()
{
    static Logger logger;
    return logger;
}

void Logger::logv(int level, const char *file, int line, const char *func, const char *fmt...)
{}

void Logger::setFileName(const std::string &filename)
{

}

void Logger::setLogLevel(const std::string &level)
{

}

void Logger::maybeRotate()
{}
