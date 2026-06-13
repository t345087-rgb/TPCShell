#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <windows.h>
#include <string>

struct BackgroundProcess {
    DWORD pid;
    HANDLE hProcess;
    std::string cmdName;
    bool isRunning;
};

// Các hàm quản lý danh sách do CHÍNH phụ trách
void addBackgroundProcess(DWORD pid, HANDLE hProcess, const char* cmdName);
void listBackgroundProcesses();
void killProcess(DWORD pid);
void stopProcess(DWORD pid);
void resumeProcess(DWORD pid);
void setupSignalHandler();

#endif
