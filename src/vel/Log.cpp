#include <fstream>
#include <sstream>
#include <iostream>

#include "vel/Log.h"

namespace vel
{
    Log* Log::instance = nullptr;
    std::string Log::logDir = "data";

    void Log::setLogDir(const std::string& dir)
    {
        Log::logDir = dir;
    }

    Log& Log::get()
    {
        if (Log::instance == nullptr)
        {
            static Log l(Log::logDir + "/log.txt");
            Log::instance = &l;
        }

        return *Log::instance;
    }

    Log::Log(std::string logFilePath) :
        filePath(logFilePath){}

    // ***NOTE*** 
    // Will not be called if program terminates before main returns 
    // (if you close cmd.exe by clicking close button for example)
    Log::~Log()
    {
        this->publishLog();
    }

    void Log::publishLog()
    {
        std::ofstream outStream(this->filePath, std::ofstream::trunc);

        for (auto& l : this->fileBuffer)
            outStream << l << std::endl;

        outStream.close();
    }

    void Log::toFile(const std::string& msg)
    {
        Log::get().fileBuffer.push_back(msg);
    }

    void Log::toCli(const std::string& msg)
    {
        std::cout << msg << std::endl;
    }

	void Log::toCliAndFile(const std::string& msg)
	{
		Log::toCli(msg);
		Log::toFile(msg);
	}

    void Log::crash(const std::string& msg)
    {
        Log::toCli(msg);
        Log::toFile(msg);
        exit(EXIT_FAILURE);
    }

    void Log::crashIfTrue(bool condition, const std::string& msg)
    {
        if (condition)
        {
            Log::toCli(msg);
            Log::toFile(msg);
            exit(EXIT_FAILURE);
        }
    }

    void Log::crashIfFalse(bool condition, const std::string& msg)
    {
        if (!condition)
        {
            Log::toCli(msg);
            Log::toFile(msg);
            exit(EXIT_FAILURE);
        }
    }

}