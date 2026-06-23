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

// process_mgr.cpp owns the original foreground process HANDLE.
// The controller owns and closes only its internal duplicated HANDLE.
bool setForegroundProcess(DWORD pid, HANDLE hProcess);
void clearForegroundProcess();

void beginBatchExecution();
void endBatchExecution();
bool isBatchCancellationRequested();
void registerBatchProcess(DWORD pid);

// Các hàm quản lý danh sách do CHÍNH phụ trách
void addBackgroundProcess(DWORD pid, HANDLE hProcess, const char* cmdName);
bool hasManagedBackgroundProcesses();
void listBackgroundProcesses();
void killProcess(DWORD pid);
void killAllProcesses();
void stopProcess(DWORD pid);
void resumeProcess(DWORD pid);
void setupSignalHandler();

#endif
