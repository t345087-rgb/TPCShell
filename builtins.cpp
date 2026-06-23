#include "builtins.h"
#include "controller.h"

#include <iostream>
#include <windows.h>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <stdexcept>

namespace {
std::vector<std::string> shellPaths;
}

void cmdHelp() {
    std::cout << "\n=== TPCShell Help ===\n";
    std::cout << "Available commands:\n\n";

    std::cout << "Built-in Commands:\n";
    std::cout << "  help              - Show this help message\n";
    std::cout << "  exit              - Exit the shell\n";
    std::cout << "  date              - Display current date\n";
    std::cout << "  time              - Display current time\n";
    std::cout << "  dir [path]        - List directory contents\n";
    std::cout << "  cd <path>         - Change directory\n";
    std::cout << "  mkdir <dir>       - Create a directory\n";
    std::cout << "  deldir <dir>      - Remove an empty directory\n";
    std::cout << "  pwd               - Display current working directory\n";
    std::cout << "  clear             - Clear the screen\n";
    std::cout << "  path              - Display TPCShell-local PATH entries\n";
    std::cout << "  addpath <dir>     - Add directory to TPCShell-local PATH\n";
    std::cout << "  delpath <dir>     - Remove directory from TPCShell-local PATH\n\n";

    std::cout << "Process Commands:\n";
    std::cout << "  list              - List background processes\n";
    std::cout << "  kill <PID>        - Terminate a background process\n";
    std::cout << "  killall           - Terminate all managed background processes\n";
    std::cout << "  stop <PID>        - Pause a background process\n";
    std::cout << "  resume <PID>      - Resume a paused process\n\n";

    std::cout << "Usage:\n";
    std::cout << "  command arg1 arg2 ...  - Run command with arguments\n";
    std::cout << "  command arg &          - Run command in background\n";
    std::cout << "========================\n\n";
}

void cmdDate() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);

    char dateStr[100];
    std::strftime(dateStr, sizeof(dateStr), "%A, %B %d, %Y", localTime);

    std::cout << "Current date: " << dateStr << "\n";
}

void cmdTime() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);

    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", localTime);

    std::cout << "Current time: " << timeStr << "\n";
}

void cmdDir(const std::vector<std::string>& args) {
    std::string displayPath = args.empty() ? "." : args[0];
    std::string searchPath = displayPath;

    // Append \*.* if path doesn't end with \ or /
    if (!searchPath.empty() && searchPath.back() != '\\' && searchPath.back() != '/') {
        searchPath += "\\*.*";
    } else {
        searchPath += "*.*";
    }

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "dir: cannot access '" << displayPath
                  << "': No such file or directory\n";
        return;
    }

    int count = 0;
    std::cout << "\n Directory of " << displayPath << "\n\n";

    do {
        std::string name = findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::cout << std::setw(30) << std::left << name << " <DIR>\n";
        } else {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;

            std::cout << std::setw(30) << std::left << name << " "
                      << std::setw(15) << std::right << fileSize.QuadPart
                      << " bytes\n";
        }

        count++;
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    int realCount = count >= 2 ? count - 2 : count;
    std::cout << "\n Total items: " << realCount << " (excluding . and ..)\n";
}

void cmdCd(const std::vector<std::string>& args) {
    if (args.empty()) {
        char currentDir[MAX_PATH];

        if (GetCurrentDirectoryA(MAX_PATH, currentDir)) {
            std::cout << "Current directory: " << currentDir << "\n";
        } else {
            std::cerr << "cd: failed to get current directory\n";
        }

        return;
    }

    std::string targetDir = args[0];

    // Handle cd .. and cd .
    if (targetDir == "..") {
        if (!SetCurrentDirectoryA("..")) {
            std::cerr << "cd: failed to move to parent directory\n";
        }
        return;
    }

    if (targetDir == ".") {
        return;
    }

    if (!SetCurrentDirectoryA(targetDir.c_str())) {
        std::cerr << "cd: " << targetDir << ": No such file or directory\n";
    }
}

void cmdMkdir(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        std::cerr << "Usage: mkdir <directory>\n";
        return;
    }

    const std::string& directory = args[0];
    if (CreateDirectoryA(directory.c_str(), NULL)) {
        std::cout << "[TPCShell] Directory created: " << directory << "\n";
        return;
    }

    std::cerr << "[TPCShell] Failed to create directory '" << directory
              << "'. Windows error: " << GetLastError() << "\n";
}

void cmdDeldir(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        std::cerr << "Usage: deldir <directory>\n";
        return;
    }

    const std::string& directory = args[0];
    if (RemoveDirectoryA(directory.c_str())) {
        std::cout << "[TPCShell] Directory removed: " << directory << "\n";
        return;
    }

    std::cerr << "[TPCShell] Failed to remove directory '" << directory
              << "'. Windows error: " << GetLastError() << "\n";
}

void cmdPwd() {
    char currentDir[MAX_PATH];

    if (GetCurrentDirectoryA(MAX_PATH, currentDir)) {
        std::cout << currentDir << "\n";
    } else {
        std::cerr << "pwd: failed to get current directory\n";
    }
}

void cmdClear() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hConsole == INVALID_HANDLE_VALUE) {
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        return;
    }

    DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD charsWritten;
    COORD topLeft = {0, 0};

    FillConsoleOutputCharacterA(
        hConsole,
        ' ',
        consoleSize,
        topLeft,
        &charsWritten
    );

    FillConsoleOutputAttribute(
        hConsole,
        csbi.wAttributes,
        consoleSize,
        topLeft,
        &charsWritten
    );

    SetConsoleCursorPosition(hConsole, topLeft);
}

void cmdPath() {
    if (shellPaths.empty()) {
        std::cout << "[TPCShell] Shell PATH is empty.\n";
        return;
    }

    std::cout << "[TPCShell] Shell PATH entries:\n";
    for (std::size_t i = 0; i < shellPaths.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << shellPaths[i] << "\n";
    }
}

void cmdAddPath(const std::string& dir) {
    if (dir.empty()) {
        std::cerr << "Usage: addpath <directory>\n";
        return;
    }

    if (std::find(shellPaths.begin(), shellPaths.end(), dir) != shellPaths.end()) {
        std::cout << "[TPCShell] PATH entry already exists: " << dir << "\n";
        return;
    }

    shellPaths.push_back(dir);
    std::cout << "[TPCShell] Added PATH entry: " << dir << "\n";
}

void cmdDelPath(const std::string& dir) {
    if (dir.empty()) {
        std::cerr << "Usage: delpath <directory>\n";
        return;
    }

    auto entry = std::find(shellPaths.begin(), shellPaths.end(), dir);
    if (entry == shellPaths.end()) {
        std::cout << "[TPCShell] PATH entry not found: " << dir << "\n";
        return;
    }

    shellPaths.erase(entry);
    std::cout << "[TPCShell] Removed PATH entry: " << dir << "\n";
}

bool isBuiltinCommand(const std::string& cmd) {
    return cmd == "help" ||
           cmd == "exit" ||
           cmd == "date" ||
           cmd == "time" ||
           cmd == "dir" ||
           cmd == "cd" ||
           cmd == "mkdir" ||
           cmd == "deldir" ||
           cmd == "pwd" ||
           cmd == "clear" ||
           cmd == "path" ||
           cmd == "addpath" ||
           cmd == "delpath" ||
           cmd == "list" ||
           cmd == "kill" ||
           cmd == "killall" ||
           cmd == "stop" ||
           cmd == "resume";
}

bool parsePidArgument(const std::vector<std::string>& args, const std::string& commandName, DWORD& pid) {
    if (args.empty()) {
        std::cerr << commandName << ": missing PID argument\n";
        return false;
    }

    try {
        size_t parsedChars = 0;
        unsigned long value = std::stoul(args[0], &parsedChars);

        if (parsedChars != args[0].length()) {
            std::cerr << commandName << ": invalid PID '" << args[0] << "'\n";
            return false;
        }

        pid = static_cast<DWORD>(value);
        return true;
    } catch (const std::exception&) {
        std::cerr << commandName << ": invalid PID '" << args[0] << "'\n";
        return false;
    }
}

void executeBuiltin(const ParsedCommand& parsed) {
    if (parsed.command == "help") {
        cmdHelp();

    } else if (parsed.command == "date") {
        cmdDate();

    } else if (parsed.command == "time") {
        cmdTime();

    } else if (parsed.command == "dir") {
        cmdDir(parsed.args);

    } else if (parsed.command == "cd") {
        cmdCd(parsed.args);

    } else if (parsed.command == "mkdir") {
        cmdMkdir(parsed.args);

    } else if (parsed.command == "deldir") {
        cmdDeldir(parsed.args);

    } else if (parsed.command == "pwd") {
        cmdPwd();

    } else if (parsed.command == "clear") {
        cmdClear();

    } else if (parsed.command == "path") {
        cmdPath();

    } else if (parsed.command == "addpath") {
        if (parsed.args.size() == 1) {
            cmdAddPath(parsed.args[0]);
        } else {
            std::cerr << "Usage: addpath <directory>\n";
        }

    } else if (parsed.command == "delpath") {
        if (parsed.args.size() == 1) {
            cmdDelPath(parsed.args[0]);
        } else {
            std::cerr << "Usage: delpath <directory>\n";
        }

    } else if (parsed.command == "list") {
        listBackgroundProcesses();

    } else if (parsed.command == "kill") {
        DWORD pid = 0;
        if (parsePidArgument(parsed.args, "kill", pid)) {
            killProcess(pid);
        }

    } else if (parsed.command == "killall") {
        if (parsed.args.empty()) {
            killAllProcesses();
        } else {
            std::cerr << "Usage: killall\n";
        }

    } else if (parsed.command == "stop") {
        DWORD pid = 0;
        if (parsePidArgument(parsed.args, "stop", pid)) {
            stopProcess(pid);
        }

    } else if (parsed.command == "resume") {
        DWORD pid = 0;
        if (parsePidArgument(parsed.args, "resume", pid)) {
            resumeProcess(pid);
        }
    }
}
