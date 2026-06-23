#include "controller.h"
#include <tlhelp32.h>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {
std::vector<BackgroundProcess> backgroundProcesses;
DWORD foregroundPid = 0;
HANDLE foregroundHandle = NULL;
SRWLOCK foregroundLock = SRWLOCK_INIT;
SRWLOCK backgroundLock = SRWLOCK_INIT;
SRWLOCK batchLock = SRWLOCK_INIT;
bool batchActive = false;
bool batchCancellationRequested = false;
int batchDepth = 0;
std::vector<DWORD> batchProcessIds;

std::vector<DWORD> findDescendantProcesses(DWORD rootPid) {
    std::vector<DWORD> descendants;
    std::vector<DWORD> queue;
    queue.push_back(rootPid);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return descendants;
    }

    PROCESSENTRY32 processEntry = {};
    processEntry.dwSize = sizeof(processEntry);
    if (!Process32First(snapshot, &processEntry)) {
        CloseHandle(snapshot);
        return descendants;
    }

    bool added = true;
    while (added) {
        added = false;

        do {
            for (DWORD parentPid : queue) {
                if (processEntry.th32ParentProcessID == parentPid &&
                    processEntry.th32ProcessID != rootPid) {
                    const bool alreadyKnown =
                        std::find(queue.begin(), queue.end(),
                                  processEntry.th32ProcessID) != queue.end();
                    if (!alreadyKnown) {
                        queue.push_back(processEntry.th32ProcessID);
                        descendants.push_back(processEntry.th32ProcessID);
                        added = true;
                    }
                    break;
                }
            }
        } while (Process32Next(snapshot, &processEntry));

        if (added) {
            Process32First(snapshot, &processEntry);
        }
    }

    CloseHandle(snapshot);
    return descendants;
}

void terminatePid(DWORD pid, UINT exitCode) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
    if (hProcess == NULL) {
        return;
    }

    TerminateProcess(hProcess, exitCode);
    WaitForSingleObject(hProcess, 5000);
    CloseHandle(hProcess);
}

void terminateProcessTree(DWORD rootPid, HANDLE rootHandle, UINT exitCode) {
    if (rootPid == 0) {
        return;
    }

    std::vector<DWORD> descendants = findDescendantProcesses(rootPid);
    for (auto process = descendants.rbegin();
         process != descendants.rend(); ++process) {
        terminatePid(*process, exitCode);
    }

    if (rootHandle != NULL && rootHandle != INVALID_HANDLE_VALUE) {
        TerminateProcess(rootHandle, exitCode);
        WaitForSingleObject(rootHandle, 5000);
        return;
    }

    terminatePid(rootPid, exitCode);
}

void removeManagedProcessByPid(DWORD pid) {
    for (auto process = backgroundProcesses.begin();
         process != backgroundProcesses.end(); ++process) {
        if (process->pid == pid) {
            CloseHandle(process->hProcess);
            backgroundProcesses.erase(process);
            return;
        }
    }
}

BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType != CTRL_C_EVENT) {
        return FALSE;
    }

    bool shouldCancelBatch = false;
    std::vector<DWORD> batchPids;
    AcquireSRWLockExclusive(&batchLock);
    if (batchActive) {
        batchCancellationRequested = true;
        shouldCancelBatch = true;
        batchPids = batchProcessIds;
    }
    ReleaseSRWLockExclusive(&batchLock);

    AcquireSRWLockShared(&foregroundLock);
    const DWORD activeForegroundPid = foregroundPid;
    HANDLE activeForegroundHandle = foregroundHandle;
    if (foregroundHandle != NULL &&
        foregroundHandle != INVALID_HANDLE_VALUE) {
        terminateProcessTree(activeForegroundPid, activeForegroundHandle, 130);
    }
    ReleaseSRWLockShared(&foregroundLock);

    if (shouldCancelBatch) {
        AcquireSRWLockExclusive(&backgroundLock);
        for (DWORD pid : batchPids) {
            for (const BackgroundProcess& process : backgroundProcesses) {
                if (process.pid == pid) {
                    terminateProcessTree(process.pid, process.hProcess, 130);
                    break;
                }
            }
            removeManagedProcessByPid(pid);
        }
        ReleaseSRWLockExclusive(&backgroundLock);
    }

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

    AcquireSRWLockExclusive(&backgroundLock);
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
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    backgroundProcesses.push_back({pid, hProcess, processName, true});
    ReleaseSRWLockExclusive(&backgroundLock);
}

void listBackgroundProcesses() {
    AcquireSRWLockExclusive(&backgroundLock);
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
            std::cerr << "[TPCShell] Failed to inspect process PID "
                      << process->pid << ". Windows error: "
                      << errorCode << '\n';
        }

        ++process;
    }

    if (backgroundProcesses.empty()) {
        std::cout << "[TPCShell] No managed background processes.\n";
        ReleaseSRWLockExclusive(&backgroundLock);
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
    ReleaseSRWLockExclusive(&backgroundLock);
}

bool hasManagedBackgroundProcesses() {
    AcquireSRWLockExclusive(&backgroundLock);
    for (auto process = backgroundProcesses.begin();
         process != backgroundProcesses.end();) {
        const DWORD waitResult = WaitForSingleObject(process->hProcess, 0);
        if (waitResult == WAIT_OBJECT_0) {
            CloseHandle(process->hProcess);
            process = backgroundProcesses.erase(process);
            continue;
        }

        ++process;
    }

    const bool hasProcesses = !backgroundProcesses.empty();
    ReleaseSRWLockExclusive(&backgroundLock);
    return hasProcesses;
}

void killProcess(DWORD pid) {
    AcquireSRWLockExclusive(&backgroundLock);
    for (auto process = backgroundProcesses.begin();
         process != backgroundProcesses.end(); ++process) {
        if (process->pid != pid) {
            continue;
        }

        const DWORD waitResult = WaitForSingleObject(process->hProcess, 0);

        if (waitResult == WAIT_OBJECT_0) {
            removeBackgroundProcess(process);
            std::cout << "[TPCShell] Process PID " << pid
                      << " has already exited and was removed from TPCShell.\n";
            ReleaseSRWLockExclusive(&backgroundLock);
            return;
        }

        if (waitResult == WAIT_FAILED) {
            const DWORD errorCode = GetLastError();
            std::cerr << "[TPCShell] Failed to inspect process PID "
                      << pid << ". Windows error: " << errorCode << '\n';
            ReleaseSRWLockExclusive(&backgroundLock);
            return;
        }

        terminateProcessTree(pid, process->hProcess, 1);
        removeBackgroundProcess(process);
        std::cout << "[TPCShell] Terminated process PID " << pid << ".\n";
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    std::cerr << "[TPCShell] Process PID " << pid
              << " is not managed by TPCShell.\n";
    ReleaseSRWLockExclusive(&backgroundLock);
}

void killAllProcesses() {
    AcquireSRWLockExclusive(&backgroundLock);
    if (backgroundProcesses.empty()) {
        std::cout << "[TPCShell] No managed background processes to terminate.\n";
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    int terminatedCount = 0;

    for (auto process = backgroundProcesses.begin();
         process != backgroundProcesses.end();) {
        const DWORD pid = process->pid;
        const DWORD waitResult = WaitForSingleObject(process->hProcess, 0);

        if (waitResult == WAIT_OBJECT_0) {
            CloseHandle(process->hProcess);
            process = backgroundProcesses.erase(process);
            continue;
        }

        if (waitResult == WAIT_FAILED) {
            const DWORD errorCode = GetLastError();
            std::cerr << "[TPCShell] Failed to inspect process PID "
                      << pid << ". Windows error: " << errorCode << '\n';
            ++process;
            continue;
        }

        terminateProcessTree(pid, process->hProcess, 1);
        CloseHandle(process->hProcess);
        process = backgroundProcesses.erase(process);
        ++terminatedCount;
    }

    std::cout << "[TPCShell] Terminated " << terminatedCount
              << " background process(es).\n";
    ReleaseSRWLockExclusive(&backgroundLock);
}

void stopProcess(DWORD pid) {
    AcquireSRWLockExclusive(&backgroundLock);
    auto processIt = backgroundProcesses.begin();
    while (processIt != backgroundProcesses.end() && processIt->pid != pid) {
        ++processIt;
    }

    if (processIt == backgroundProcesses.end()) {
        std::cout << "Error: PID " << pid
                  << " is not managed by TPCShell." << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    DWORD waitResult = WaitForSingleObject(processIt->hProcess, 0);
    if (waitResult == WAIT_OBJECT_0) {
        CloseHandle(processIt->hProcess);
        backgroundProcesses.erase(processIt);
        std::cout << "Process PID " << pid
                  << " has already exited and was removed from TPCShell."
                  << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    if (waitResult == WAIT_FAILED) {
        std::cout << "Error: Failed to inspect PID " << pid
                  << ". Windows error: " << GetLastError() << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    if (waitResult != WAIT_TIMEOUT) {
        std::cout << "Error: Unexpected wait result " << waitResult
                  << " for PID " << pid << "." << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    if (!processIt->isRunning) {
        std::cout << "Process PID " << pid << " is already stopped."
                  << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cout << "Error: Failed to create a thread snapshot for PID "
                  << pid << ". Windows error: " << GetLastError()
                  << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    THREADENTRY32 threadEntry = {};
    threadEntry.dwSize = sizeof(threadEntry);
    if (!Thread32First(snapshot, &threadEntry)) {
        DWORD error = GetLastError();
        CloseHandle(snapshot);
        std::cout << "Error: Failed to enumerate threads for PID " << pid
                  << ". Windows error: " << error << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
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
                ReleaseSRWLockExclusive(&backgroundLock);
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
                ReleaseSRWLockExclusive(&backgroundLock);
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
                ReleaseSRWLockExclusive(&backgroundLock);
                return;
            }

            break;
        }
    }

    if (!foundThread) {
        std::cout << "Error: No threads were found for PID " << pid << "."
                  << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    processIt->isRunning = false;
    std::cout << "Process PID " << pid << " stopped successfully."
              << std::endl;
    ReleaseSRWLockExclusive(&backgroundLock);
}

void resumeProcess(DWORD pid) {
    AcquireSRWLockExclusive(&backgroundLock);
    auto processIt = backgroundProcesses.begin();
    while (processIt != backgroundProcesses.end() && processIt->pid != pid) {
        ++processIt;
    }

    if (processIt == backgroundProcesses.end()) {
        std::cout << "Error: PID " << pid
                  << " is not managed by TPCShell." << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    DWORD waitResult = WaitForSingleObject(processIt->hProcess, 0);
    if (waitResult == WAIT_OBJECT_0) {
        CloseHandle(processIt->hProcess);
        backgroundProcesses.erase(processIt);
        std::cout << "Process PID " << pid
                  << " has already exited and was removed from TPCShell."
                  << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    if (waitResult == WAIT_FAILED) {
        std::cout << "Error: Failed to inspect PID " << pid
                  << ". Windows error: " << GetLastError() << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    if (waitResult != WAIT_TIMEOUT) {
        std::cout << "Error: Unexpected wait result " << waitResult
                  << " for PID " << pid << "." << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    if (processIt->isRunning) {
        std::cout << "Process PID " << pid << " is already running."
                  << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cout << "Error: Failed to create a thread snapshot for PID "
                  << pid << ". Windows error: " << GetLastError()
                  << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    THREADENTRY32 threadEntry = {};
    threadEntry.dwSize = sizeof(threadEntry);
    if (!Thread32First(snapshot, &threadEntry)) {
        DWORD error = GetLastError();
        CloseHandle(snapshot);
        std::cout << "Error: Failed to enumerate threads for PID " << pid
                  << ". Windows error: " << error << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
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
                ReleaseSRWLockExclusive(&backgroundLock);
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
                ReleaseSRWLockExclusive(&backgroundLock);
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
                ReleaseSRWLockExclusive(&backgroundLock);
                return;
            }

            break;
        }
    }

    if (!foundThread) {
        std::cout << "Error: No threads were found for PID " << pid << "."
                  << std::endl;
        ReleaseSRWLockExclusive(&backgroundLock);
        return;
    }

    processIt->isRunning = true;
    std::cout << "Process PID " << pid << " resumed successfully."
              << std::endl;
    ReleaseSRWLockExclusive(&backgroundLock);
}

void beginBatchExecution() {
    AcquireSRWLockExclusive(&batchLock);
    if (batchDepth == 0) {
        batchActive = true;
        batchCancellationRequested = false;
        batchProcessIds.clear();
    }
    ++batchDepth;
    ReleaseSRWLockExclusive(&batchLock);
}

void endBatchExecution() {
    AcquireSRWLockExclusive(&batchLock);
    if (batchDepth > 0) {
        --batchDepth;
    }
    if (batchDepth == 0) {
        batchActive = false;
        batchProcessIds.clear();
    }
    ReleaseSRWLockExclusive(&batchLock);
}

bool isBatchCancellationRequested() {
    AcquireSRWLockShared(&batchLock);
    const bool requested = batchCancellationRequested;
    ReleaseSRWLockShared(&batchLock);
    return requested;
}

void registerBatchProcess(DWORD pid) {
    AcquireSRWLockExclusive(&batchLock);
    if (batchActive) {
        batchProcessIds.push_back(pid);
    }
    ReleaseSRWLockExclusive(&batchLock);
}

void setupSignalHandler() {
    if (!SetConsoleCtrlHandler(consoleCtrlHandler, TRUE)) {
        const DWORD errorCode = GetLastError();
        std::cerr << "[TPCShell] Failed to register the console control "
                  << "handler. Windows error: " << errorCode << '\n';
    }
}
