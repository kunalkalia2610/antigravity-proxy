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

        static std::string GetTodayLogName() {
            auto now = std::time(nullptr);
            struct tm tm;
            localtime_s(&tm, &now);
            std::ostringstream oss;
            oss << "proxy-" << std::put_time(&tm, "%Y%m%d") << ".log";
            return oss.str();
        }

        static bool IsLogOverLimit(const std::string& path, ULONGLONG maxBytes) {
            WIN32_FILE_ATTRIBUTE_DATA data{};
            if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &data)) {
                return false;
            }
            ULONGLONG size = (static_cast<ULONGLONG>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
            return size >= maxBytes;
        }

        static void CleanupOldLogs(const std::string& todayLog) {
            // 删除非当天日志，只保留当前日期的日志文件
            WIN32_FIND_DATAA findData{};
            HANDLE hFind = FindFirstFileA("proxy-*.log", &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        continue;
                    }
                    if (todayLog != findData.cFileName) {
                        DeleteFileA(findData.cFileName);
                    }
                } while (FindNextFileA(hFind, &findData));
                FindClose(hFind);
            }
            if (todayLog != "proxy.log") {
                DeleteFileA("proxy.log");
            }
            DeleteFileA("proxy.log.1");
        }

        static void WriteToFile(const std::string& message) {
            static std::mutex mtx;
            std::lock_guard<std::mutex> lock(mtx);
            // 按日期写日志并清理旧文件，避免历史日志堆积
            static std::string s_todayLog;
            static ULONGLONG s_lastCheckTick = 0;
            static bool s_dropForToday = false;
            static const ULONGLONG kMaxLogBytes = 100ull * 1024 * 1024; // 100MB
            static const ULONGLONG kCheckIntervalMs = 60ull * 60 * 1000; // 1 小时
            std::string todayLog = GetTodayLogName();
            if (s_todayLog != todayLog) {
                s_todayLog = todayLog;
                s_lastCheckTick = 0;
                s_dropForToday = false;
                CleanupOldLogs(s_todayLog);
            }
            if (s_dropForToday) {
                return;
            }
            ULONGLONG nowTick = GetTickCount64();
            if (s_lastCheckTick == 0 || nowTick - s_lastCheckTick >= kCheckIntervalMs) {
                s_lastCheckTick = nowTick;
                if (IsLogOverLimit(s_todayLog, kMaxLogBytes)) {
                    // 当天日志超过上限后直接丢弃，次日恢复
                    s_dropForToday = true;
                    return;
                }
            }
            std::ofstream logFile(s_todayLog, std::ios::app);
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
