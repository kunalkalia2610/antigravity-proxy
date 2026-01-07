#pragma once
#include <fstream>
#include <mutex>
#include <string>
#include <iostream>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <ctime>
#include <iomanip>
#include <sstream>

namespace Core {
    class Logger {
    private:
        static std::string GetTimestamp() {
            auto now = std::time(nullptr);
            struct tm tm;
            localtime_s(&tm, &now);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            return oss.str();
        }

        static std::string GetPidTidPrefix() {
            // 在多进程/多线程混写同一个日志文件时，PID/TID 有助于定位来源
            DWORD pid = GetCurrentProcessId();
            DWORD tid = GetCurrentThreadId();
            return "[PID:" + std::to_string(pid) + "][TID:" + std::to_string(tid) + "]";
        }

        static void WriteToFile(const std::string& message) {
            static std::mutex mtx;
            std::lock_guard<std::mutex> lock(mtx);
            std::ofstream logFile("proxy.log", std::ios::app);
            if (logFile.is_open()) {
                logFile << message << "\n";
            }
        }

    public:
        static void Log(const std::string& message) {
            WriteToFile("[" + GetTimestamp() + "] " + GetPidTidPrefix() + " " + message);
        }

        static void Error(const std::string& message) {
            WriteToFile("[" + GetTimestamp() + "] " + GetPidTidPrefix() + " [错误] " + message);
        }

        static void Info(const std::string& message) {
            WriteToFile("[" + GetTimestamp() + "] " + GetPidTidPrefix() + " [信息] " + message);
        }

        static void Warn(const std::string& message) {
            WriteToFile("[" + GetTimestamp() + "] " + GetPidTidPrefix() + " [警告] " + message);
        }

        static void Debug(const std::string& message) {
            WriteToFile("[" + GetTimestamp() + "] " + GetPidTidPrefix() + " [调试] " + message);
        }
    };
}
