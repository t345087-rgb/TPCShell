#ifndef PROCESS_MGR_H
#define PROCESS_MGR_H

#include <windows.h>
#include <string>

enum class CommandResult {
    Continue,
    Exit
};

// Hàm thực thi lệnh do PHƯƠNG phụ trách
void executeCommand(const char* cmdName, char* argv[], bool isBackground);
CommandResult executeShellLine(const std::string& input);

// Hàm đọc và chạy file script *.bat do PHƯƠNG phụ trách
void executeBatchFile(const char* filePath);

#endif
