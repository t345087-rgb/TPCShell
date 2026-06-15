#include "controller.h"
#include <TlHelp32.h>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {
std::vector<BackgroundProcess> backgroundProcesses;
DWORD foregroundPid = 0;
HANDLE foregroundHandle = NULL;
SRWLOCK foregroundLock = SRWLOCK_INIT;

BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType != CTRL_C_EVENT) {
        return FALSE;
    }

    AcquireSRWLockShared(&foregroundLock);
    if (foregroundHandle != NULL &&
        foregroundHandle != INVALID_HANDLE_VALUE) {
        TerminateProcess(foregroundHandle, 130);
    }
    ReleaseSRWLockShared(&foregroundLock);

    return TRUE;
}

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

void removeBackgroundProcess(
    std::vector<BackgroundProcess>::iterator process) {
    CloseHandle(process->hProcess);
    backgroundProcesses.erase(process);
}

void rollbackSuspendedThreads(const std::vector<DWORD>& threadIds, DWORD pid) {
    for (DWORD threadId : threadIds) {
        HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadId);
        if (hThread == NULL) {
            std::cout << "Error: Failed to reopen thread " << threadId
                      << " while rolling back stop for PID " << pid
                      << ". Windows error: " << GetLastError() << std::endl;
            continue;
        }

        if (ResumeThread(hThread) == static_cast<DWORD>(-1)) {
            std::cout << "Error: Failed to resume thread " << threadId
                      << " while rolling back stop for PID " << pid
                      << ". Windows error: " << GetLastError() << std::endl;
        }

        CloseHandle(hThread);
    }
}

void rollbackResumedThreads(const std::vector<DWORD>& threadIds, DWORD pid) {
    for (DWORD threadId : threadIds) {
        HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadId);
        if (hThread == NULL) {
            std::cout << "Error: Failed to reopen thread " << threadId
                      << " while rolling back resume for PID " << pid
                      << ". Windows error: " << GetLastError() << std::endl;
            continue;
        }

        if (SuspendThread(hThread) == static_cast<DWORD>(-1)) {
            std::cout << "Error: Failed to suspend thread " << threadId
                      << " while rolling back resume for PID " << pid
                      << ". Windows error: " << GetLastError() << std::endl;
        }

        CloseHandle(hThread);
    }
}
}

bool setForegroundProcess(DWORD pid, HANDLE hProcess) {
    if (pid == 0 || hProcess == NULL || hProcess == INVALID_HANDLE_VALUE) {
        return false;
    }

    HANDLE duplicatedHandle = NULL;
    if (!DuplicateHandle(
            GetCurrentProcess(),
            hProcess,
            GetCurrentProcess(),
            &duplicatedHandle,
            PROCESS_TERMINATE | SYNCHRONIZE,
            FALSE,
            0)) {
        const DWORD errorCode = GetLastError();
        std::cerr << "[TPCShell] Failed to duplicate the foreground process "
                  << "handle for PID " << pid
                  << ". Windows error: " << errorCode << '\n';
        return false;
    }

    AcquireSRWLockExclusive(&foregroundLock);
    if (foregroundHandle != NULL &&
        foregroundHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(foregroundHandle);
    }
    foregroundHandle = duplicatedHandle;
    foregroundPid = pid;
    ReleaseSRWLockExclusive(&foregroundLock);

    return true;
}

void clearForegroundProcess() {
    AcquireSRWLockExclusive(&foregroundLock);
    if (foregroundHandle != NULL &&
        foregroundHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(foregroundHandle);
    }
    foregroundHandle = NULL;
    foregroundPid = 0;
    ReleaseSRWLockExclusive(&foregroundLock);
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
    for (auto process = backgroundProcesses.begin();
         process != backgroundProcesses.end(); ++process) {
        if (process->pid != pid) {
            continue;
        }

        const DWORD waitResult = WaitForSingleObject(process->hProcess, 0);

        if (waitResult == WAIT_OBJECT_0) {
            removeBackgroundProcess(process);
            std::cout << "[TPCShell] Tien trinh PID " << pid
                      << " da ket thuc truoc do va da duoc xoa khoi danh sach.\n";
            return;
        }

        if (waitResult == WAIT_FAILED) {
            const DWORD errorCode = GetLastError();
            std::cerr << "[TPCShell] Khong the kiem tra tien trinh PID "
                      << pid << ". Ma loi Windows: " << errorCode << '\n';
            return;
        }

        if (!TerminateProcess(process->hProcess, 1)) {
            const DWORD errorCode = GetLastError();
            std::cerr << "[TPCShell] Khong the ket thuc tien trinh PID "
                      << pid << ". Ma loi Windows: " << errorCode << '\n';
            return;
        }

        const DWORD completionWait =
            WaitForSingleObject(process->hProcess, INFINITE);
        if (completionWait == WAIT_FAILED) {
            const DWORD errorCode = GetLastError();
            std::cerr << "[TPCShell] Canh bao: khong the cho tien trinh PID "
                      << pid << " ket thuc. Ma loi Windows: "
                      << errorCode << '\n';
        }

        removeBackgroundProcess(process);
        std::cout << "[TPCShell] Da ket thuc tien trinh PID " << pid << ".\n";
        return;
    }

    std::cerr << "[TPCShell] Khong tim thay tien trinh PID " << pid
              << " trong danh sach tien trinh ngam duoc quan ly.\n";
}

void stopProcess(DWORD pid) {
    auto processIt = backgroundProcesses.begin();
    while (processIt != backgroundProcesses.end() && processIt->pid != pid) {
        ++processIt;
    }

    if (processIt == backgroundProcesses.end()) {
        std::cout << "Error: PID " << pid
                  << " is not managed by TPCShell." << std::endl;
        return;
    }

    DWORD waitResult = WaitForSingleObject(processIt->hProcess, 0);
    if (waitResult == WAIT_OBJECT_0) {
        CloseHandle(processIt->hProcess);
        backgroundProcesses.erase(processIt);
        std::cout << "Process PID " << pid
                  << " has already exited and was removed from TPCShell."
                  << std::endl;
        return;
    }

    if (waitResult == WAIT_FAILED) {
        std::cout << "Error: Failed to inspect PID " << pid
                  << ". Windows error: " << GetLastError() << std::endl;
        return;
    }

    if (waitResult != WAIT_TIMEOUT) {
        std::cout << "Error: Unexpected wait result " << waitResult
                  << " for PID " << pid << "." << std::endl;
        return;
    }

    if (!processIt->isRunning) {
        std::cout << "Process PID " << pid << " is already stopped."
                  << std::endl;
        return;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cout << "Error: Failed to create a thread snapshot for PID "
                  << pid << ". Windows error: " << GetLastError()
                  << std::endl;
        return;
    }

    THREADENTRY32 threadEntry = {};
    threadEntry.dwSize = sizeof(threadEntry);
    if (!Thread32First(snapshot, &threadEntry)) {
        DWORD error = GetLastError();
        CloseHandle(snapshot);
        std::cout << "Error: Failed to enumerate threads for PID " << pid
                  << ". Windows error: " << error << std::endl;
        return;
    }

    std::vector<DWORD> suspendedThreadIds;
    bool foundThread = false;

    for (;;) {
        if (threadEntry.th32OwnerProcessID == pid) {
            foundThread = true;

            HANDLE hThread = OpenThread(
                THREAD_SUSPEND_RESUME,
                FALSE,
                threadEntry.th32ThreadID
            );
            if (hThread == NULL) {
                DWORD error = GetLastError();
                CloseHandle(snapshot);
                std::cout << "Error: Failed to open thread "
                          << threadEntry.th32ThreadID << " for PID " << pid
                          << ". Windows error: " << error << std::endl;
                rollbackSuspendedThreads(suspendedThreadIds, pid);
                return;
            }

            if (SuspendThread(hThread) == static_cast<DWORD>(-1)) {
                DWORD error = GetLastError();
                CloseHandle(hThread);
                CloseHandle(snapshot);
                std::cout << "Error: Failed to suspend thread "
                          << threadEntry.th32ThreadID << " for PID " << pid
                          << ". Windows error: " << error << std::endl;
                rollbackSuspendedThreads(suspendedThreadIds, pid);
                return;
            }

            suspendedThreadIds.push_back(threadEntry.th32ThreadID);
            CloseHandle(hThread);
        }

        if (!Thread32Next(snapshot, &threadEntry)) {
            DWORD error = GetLastError();
            CloseHandle(snapshot);

            if (error != ERROR_NO_MORE_FILES) {
                std::cout << "Error: Failed while enumerating threads for PID "
                          << pid << ". Windows error: " << error << std::endl;
                rollbackSuspendedThreads(suspendedThreadIds, pid);
                return;
            }

            break;
        }
    }

    if (!foundThread) {
        std::cout << "Error: No threads were found for PID " << pid << "."
                  << std::endl;
        return;
    }

    processIt->isRunning = false;
    std::cout << "Process PID " << pid << " stopped successfully."
              << std::endl;
}

void resumeProcess(DWORD pid) {
    auto processIt = backgroundProcesses.begin();
    while (processIt != backgroundProcesses.end() && processIt->pid != pid) {
        ++processIt;
    }

    if (processIt == backgroundProcesses.end()) {
        std::cout << "Error: PID " << pid
                  << " is not managed by TPCShell." << std::endl;
        return;
    }

    DWORD waitResult = WaitForSingleObject(processIt->hProcess, 0);
    if (waitResult == WAIT_OBJECT_0) {
        CloseHandle(processIt->hProcess);
        backgroundProcesses.erase(processIt);
        std::cout << "Process PID " << pid
                  << " has already exited and was removed from TPCShell."
                  << std::endl;
        return;
    }

    if (waitResult == WAIT_FAILED) {
        std::cout << "Error: Failed to inspect PID " << pid
                  << ". Windows error: " << GetLastError() << std::endl;
        return;
    }

    if (waitResult != WAIT_TIMEOUT) {
        std::cout << "Error: Unexpected wait result " << waitResult
                  << " for PID " << pid << "." << std::endl;
        return;
    }

    if (processIt->isRunning) {
        std::cout << "Process PID " << pid << " is already running."
                  << std::endl;
        return;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cout << "Error: Failed to create a thread snapshot for PID "
                  << pid << ". Windows error: " << GetLastError()
                  << std::endl;
        return;
    }

    THREADENTRY32 threadEntry = {};
    threadEntry.dwSize = sizeof(threadEntry);
    if (!Thread32First(snapshot, &threadEntry)) {
        DWORD error = GetLastError();
        CloseHandle(snapshot);
        std::cout << "Error: Failed to enumerate threads for PID " << pid
                  << ". Windows error: " << error << std::endl;
        return;
    }

    std::vector<DWORD> resumedThreadIds;
    bool foundThread = false;

    for (;;) {
        if (threadEntry.th32OwnerProcessID == pid) {
            foundThread = true;

            HANDLE hThread = OpenThread(
                THREAD_SUSPEND_RESUME,
                FALSE,
                threadEntry.th32ThreadID
            );
            if (hThread == NULL) {
                DWORD error = GetLastError();
                CloseHandle(snapshot);
                std::cout << "Error: Failed to open thread "
                          << threadEntry.th32ThreadID << " for PID " << pid
                          << ". Windows error: " << error << std::endl;
                rollbackResumedThreads(resumedThreadIds, pid);
                return;
            }

            if (ResumeThread(hThread) == static_cast<DWORD>(-1)) {
                DWORD error = GetLastError();
                CloseHandle(hThread);
                CloseHandle(snapshot);
                std::cout << "Error: Failed to resume thread "
                          << threadEntry.th32ThreadID << " for PID " << pid
                          << ". Windows error: " << error << std::endl;
                rollbackResumedThreads(resumedThreadIds, pid);
                return;
            }

            resumedThreadIds.push_back(threadEntry.th32ThreadID);
            CloseHandle(hThread);
        }

        if (!Thread32Next(snapshot, &threadEntry)) {
            DWORD error = GetLastError();
            CloseHandle(snapshot);

            if (error != ERROR_NO_MORE_FILES) {
                std::cout << "Error: Failed while enumerating threads for PID "
                          << pid << ". Windows error: " << error << std::endl;
                rollbackResumedThreads(resumedThreadIds, pid);
                return;
            }

            break;
        }
    }

    if (!foundThread) {
        std::cout << "Error: No threads were found for PID " << pid << "."
                  << std::endl;
        return;
    }

    processIt->isRunning = true;
    std::cout << "Process PID " << pid << " resumed successfully."
              << std::endl;
}

void setupSignalHandler() {
    if (!SetConsoleCtrlHandler(consoleCtrlHandler, TRUE)) {
        const DWORD errorCode = GetLastError();
        std::cerr << "[TPCShell] Failed to register the console control "
                  << "handler. Windows error: " << errorCode << '\n';
    }
}
