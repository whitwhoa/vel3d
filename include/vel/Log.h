#pragma once

#include <iostream>
#include <string>
#include <deque>
#include <thread>


namespace vel 
{
    class Log {
        
    private:

        std::deque<std::string>		fileBuffer;
        std::string					filePath;

                                    Log(std::string logFilePath);
        static Log*					instance;
        static Log&					get();
        void						publishLog();

    public:
                            ~Log();
                            Log(Log const&) = delete;
        void				operator=(Log const&) = delete;

        static void			toFile(const std::string& msg);
        static void			toCli(const std::string& msg);
		static void			toCliAndFile(const std::string& msg);
        static void			crash(const std::string& msg);
        static void			crashIfTrue(bool condition, const std::string& message);
        static void			crashIfFalse(bool condition, const std::string& message);
    };
}

// Check if DEBUG_LOG is defined
#ifdef DEBUG_LOG

// Define macros that call the corresponding Log class methods
#define LOG_TO_FILE(msg) vel::Log::toFile(msg)
#define LOG_TO_CLI(msg) vel::Log::toCli(msg)
#define LOG_TO_CLI_AND_FILE(msg) vel::Log::toCliAndFile(msg)
#define LOG_CRASH(msg) vel::Log::crash(msg)
#define LOG_CRASH_IF_TRUE(condition, msg) vel::Log::crashIfTrue(condition, msg)
#define LOG_CRASH_IF_FALSE(condition, msg) vel::Log::crashIfFalse(condition, msg)

#else

// Define empty macros that do nothing when DEBUG_LOG is not defined
#define LOG_TO_FILE(msg)
#define LOG_TO_CLI(msg)
#define LOG_TO_CLI_AND_FILE(msg)
#define LOG_CRASH(msg)
#define LOG_CRASH_IF_TRUE(condition, msg)
#define LOG_CRASH_IF_FALSE(condition, msg)

#endif