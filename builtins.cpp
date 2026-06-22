#include "builtins.h"
#include <iostream>
#include <windows.h>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <iomanip>

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
    std::cout << "  path              - Display PATH environment variable\n";
    std::cout << "  addpath <dir>     - Add directory to PATH\n";
    std::cout << "  delpath <dir>     - Remove directory from PATH\n\n";
    
    std::cout << "Process Commands:\n";
    std::cout << "  list              - List background processes\n";
    std::cout << "  kill <PID>        - Terminate a background process\n";
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
    std::string searchPath = ".";
    
    if (!args.empty()) {
        searchPath = args[0];
    }
    
    // Append \*.* if path doesn't end with \ or /
    if (!searchPath.empty() && searchPath.back() != '\\' && searchPath.back() != '/') {
        searchPath += "\\*.*";
    } else {
        searchPath += "*.*";
    }
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "dir: cannot access '" << args.empty() ? "." : args[0] << "': No such file or directory\n";
        return;
    }
    
    int count = 0;
    std::cout << "\n Directory of " << (args.empty() ? "." : args[0]) << "\n\n";
    
    do {
        std::string name = findData.cFileName;
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::cout << std::setw(30) << std::left << name << " <DIR>\n";
        } else {
            // File size
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            std::cout << std::setw(30) << std::left << name << " " 
                      << std::setw(15) << std::right << fileSize.QuadPart << " bytes\n";
        }
        count++;
    } while (FindNextFileA(hFind, &findData));
    
    FindClose(hFind);
    std::cout << "\n Total items: " << count - 2 << " (excluding . and ..)\n";
}

void cmdCd(const std::vector<std::string>& args) {
    if (args.empty()) {
        // Show current directory
        char currentDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, currentDir);
        std::cout << "Current directory: " << currentDir << "\n";
        return;
    }
    
    std::string targetDir = args[0];
    
    // Handle cd.. and cd.
    if (targetDir == "..") {
        SetCurrentDirectoryA("..");
        return;
    }
    
    if (targetDir == ".") {
        return;
    }
    
    // Try to change directory
    if (!SetCurrentDirectoryA(targetDir.c_str())) {
        std::cerr << "cd: " << targetDir << ": No such file or directory\n";
    }
}

void cmdPath() {
    char* pathEnv = getenv("PATH");
    if (pathEnv) {
        std::cout << "\nPATH environment variable:\n";
        
        std::string pathStr(pathEnv);
        std::string delimiter = ";";
        size_t pos = 0;
        int count = 1;
        
        while ((pos = pathStr.find(delimiter)) != std::string::npos) {
            std::cout << "  " << count++ << ". " << pathStr.substr(0, pos) << "\n";
            pathStr.erase(0, pos + delimiter.length());
        }
        std::cout << "  " << count << ". " << pathStr << "\n\n";
    } else {
        std::cout << "PATH environment variable is not set.\n";
    }
}

void cmdAddPath(const std::string& dir) {
    if (dir.empty()) {
        std::cerr << "addpath: missing directory argument\n";
        return;
    }
    
    char* pathEnv = getenv("PATH");
    std::string newPath;
    
    if (pathEnv) {
        newPath = std::string(pathEnv) + ";" + dir;
    } else {
        newPath = dir;
    }
    
    if (_putenv_s("PATH", newPath.c_str()) == 0) {
        std::cout << "Added '" << dir << "' to PATH\n";
    } else {
        std::cerr << "addpath: failed to add directory\n";
    }
}

void cmdDelPath(const std::string& dir) {
    if (dir.empty()) {
        std::cerr << "delpath: missing directory argument\n";
        return;
    }
    
    char* pathEnv = getenv("PATH");
    if (!pathEnv) {
        std::cout << "PATH is already empty\n";
        return;
    }
    
    std::string pathStr(pathEnv);
    std::string delimiter = ";";
    std::string newPath = "";
    
    size_t pos = 0;
    bool first = true;
    
    while ((pos = pathStr.find(delimiter)) != std::string::npos) {
        std::string token = pathStr.substr(0, pos);
        if (token != dir) {
            if (!first) newPath += ";";
            newPath += token;
            first = false;
        }
        pathStr.erase(0, pos + delimiter.length());
    }
    
    // Handle last token
    if (!pathStr.empty() && pathStr != dir) {
        if (!first) newPath += ";";
        newPath += pathStr;
    }
    
    if (_putenv_s("PATH", newPath.c_str()) == 0) {
        std::cout << "Removed '" << dir << "' from PATH\n";
    } else {
        std::cerr << "delpath: failed to remove directory\n";
    }
}

bool isBuiltinCommand(const std::string& cmd) {
    return cmd == "help" || cmd == "exit" || cmd == "date" || cmd == "time" ||
           cmd == "dir" || cmd == "cd" || cmd == "path" || 
           cmd == "addpath" || cmd == "delpath";
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
    } else if (parsed.command == "path") {
        cmdPath();
    } else if (parsed.command == "addpath") {
        if (!parsed.args.empty()) {
            cmdAddPath(parsed.args[0]);
        } else {
            std::cerr << "addpath: missing argument\n";
        }
    } else if (parsed.command == "delpath") {
        if (!parsed.args.empty()) {
            cmdDelPath(parsed.args[0]);
        } else {
            std::cerr << "delpath: missing argument\n";
        }
    }
}