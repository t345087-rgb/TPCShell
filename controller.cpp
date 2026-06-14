#include "controller.h"
#include <iomanip>
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
    for (auto process = backgroundProcesses.begin();
         process != backgroundProcesses.end();) {
        const DWORD waitResult = WaitForSingleObject(process->hProcess, 0);

        if (waitResult == WAIT_OBJECT_0) {
            CloseHandle(process->hProcess);
            process = backgroundProcesses.erase(process);
            continue;
        }

        if (waitResult == WAIT_FAILED) {
            const DWORD errorCode = GetLastError();
            std::cerr << "[TPCShell] Khong the kiem tra tien trinh PID "
                      << process->pid << ". Ma loi Windows: "
                      << errorCode << '\n';
        }

        ++process;
    }

    if (backgroundProcesses.empty()) {
        std::cout
            << "[TPCShell] Khong co tien trinh ngam nao dang duoc quan ly.\n";
        return;
    }

    std::cout << std::left
              << std::setw(12) << "PID"
              << std::setw(24) << "NAME"
              << "STATUS\n";

    for (const BackgroundProcess& process : backgroundProcesses) {
        std::cout << std::left
                  << std::setw(12) << process.pid
                  << std::setw(24) << process.cmdName
                  << (process.isRunning ? "RUNNING" : "STOPPED")
                  << '\n';
    }
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
