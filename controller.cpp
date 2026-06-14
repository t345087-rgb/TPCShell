#include "controller.h"
#include <iostream>
#include <vector>

namespace {
std::vector<BackgroundProcess> backgroundProcesses;

bool isHandleOwnedByAnotherRecord(HANDLE hProcess, std::size_t excludedIndex) {
    if (hProcess == NULL || hProcess == INVALID_HANDLE_VALUE) {
        return false;
    }

    for (std::size_t i = 0; i < backgroundProcesses.size(); ++i) {
        if (i != excludedIndex && backgroundProcesses[i].hProcess == hProcess) {
            return true;
        }
    }

    return false;
}
}

void addBackgroundProcess(DWORD pid, HANDLE hProcess, const char* cmdName) {
    if (pid == 0 || hProcess == NULL || hProcess == INVALID_HANDLE_VALUE) {
        return;
    }

    const std::string processName = (cmdName != nullptr) ? cmdName : "";

    for (std::size_t i = 0; i < backgroundProcesses.size(); ++i) {
        BackgroundProcess& process = backgroundProcesses[i];
        if (process.pid != pid) {
            continue;
        }

        HANDLE previousHandle = process.hProcess;
        process.cmdName = processName;
        process.hProcess = hProcess;
        process.isRunning = true;

        if (previousHandle != hProcess &&
            previousHandle != NULL &&
            previousHandle != INVALID_HANDLE_VALUE &&
            !isHandleOwnedByAnotherRecord(previousHandle, i)) {
            CloseHandle(previousHandle);
        }
        return;
    }

    backgroundProcesses.push_back({pid, hProcess, processName, true});
}

void listBackgroundProcesses() {
    // CHÍNH VIẾT CODE IN DANH SÁCH
}

void killProcess(DWORD pid) {
    // CHÍNH VIẾT CODE TERMINATEPROCESS
}

void stopProcess(DWORD pid) {
    // CHÍNH VIẾT CODE SUSPENDTHREAD
}

void resumeProcess(DWORD pid) {
    // CHÍNH VIẾT CODE RESUMETHREAD
}

void setupSignalHandler() {
    // CHÍNH VIẾT CODE SETCONSOLECTRLHANDLER
}
