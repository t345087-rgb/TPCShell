#include "process_mgr.h"
#include "controller.h"
#include "parser.h"
#include "builtins.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cctype>

namespace {
bool containsWhitespace(const std::string& value) {
    return value.find_first_of(" \t") != std::string::npos;
}

std::string quoteIfNeeded(const std::string& value) {
    if (containsWhitespace(value)) {
        return "\"" + value + "\"";
    }

    return value;
}

std::string trim(const std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

bool fileExistsAndIsNotDirectory(const std::string& path) {
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool hasBatchExtension(const std::string& path) {
    const std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return false;
    }

    const std::string extension = toLower(path.substr(dot));
    return extension == ".bat" || extension == ".cmd";
}

bool resolveBatchFile(const std::string& command, std::string& batchPath) {
    batchPath.clear();

    if (hasBatchExtension(command) && fileExistsAndIsNotDirectory(command)) {
        batchPath = command;
        return true;
    }

    std::string resolvedCommand;
    if (resolveCommandFromShellPath(command, resolvedCommand) &&
        hasBatchExtension(resolvedCommand)) {
        batchPath = resolvedCommand;
        return true;
    }

    return false;
}

bool isCommentLine(const std::string& line) {
    const std::string lowerLine = toLower(line);
    return line.empty() ||
           line[0] == '#' ||
           lowerLine == "rem" ||
           lowerLine.rfind("rem ", 0) == 0;
}
}

void executeCommand(const char* cmdName, char* argv[], bool isBackground) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // CHUẨN HÓA THAM SỐ: Ghép mảng argv[] thành một chuỗi CommandLine duy nhất cho Windows API
    std::string fullCommandLine = "";
    if (argv != NULL && argv[0] != NULL) {
        std::string executable = argv[0];
        std::string resolvedExecutable;
        if (resolveCommandFromShellPath(executable, resolvedExecutable)) {
            executable = resolvedExecutable;
        }

        fullCommandLine = quoteIfNeeded(executable);

        int i = 0;
        while (argv[++i] != NULL) {
            fullCommandLine += " ";
            fullCommandLine += argv[i];
        }
    } else {
        // Nếu bộ Parser của Tuấn chưa truyền argv, tạm thời lấy cmdName làm dòng lệnh
        std::string executable = cmdName != NULL ? cmdName : "";
        std::string resolvedExecutable;
        if (resolveCommandFromShellPath(executable, resolvedExecutable)) {
            executable = resolvedExecutable;
        }
        fullCommandLine = quoteIfNeeded(executable);
    }

    // Chuyển đổi chuỗi thành mảng char có thể chỉnh sửa (LPSTR) theo yêu cầu của CreateProcessA
    char* lpCommandLine = new char[fullCommandLine.size() + 1];
    strcpy_s(lpCommandLine, fullCommandLine.size() + 1, fullCommandLine.c_str());

    const DWORD creationFlags = isBackground ? CREATE_NEW_PROCESS_GROUP : 0;

    if (isBackground) {
        SetConsoleCtrlHandler(NULL, TRUE);
    }

    // Gọi Windows API để tạo tiến trình con
    const BOOL processCreated = CreateProcessA(
        NULL,               // Ứng dụng thực thi trực tiếp từ CommandLine
        lpCommandLine,      // Toàn bộ dòng lệnh (bao gồm cả tham số)
        NULL,               // Thuộc tính bảo mật tiến trình
        NULL,               // Thuộc tính bảo mật luồng
        FALSE,              // Không kế thừa handle
        creationFlags,      // Background tách process group để tránh Ctrl+C foreground
        NULL,               // Sử dụng môi trường của cha
        NULL,               // Sử dụng thư mục hiện hành của cha
        &si,                // Thông tin cấu hình cửa sổ ban đầu
        &pi                 // Nhận thông tin handle/PID trả về từ OS
    );

    const DWORD createError = processCreated ? ERROR_SUCCESS : GetLastError();

    if (isBackground) {
        SetConsoleCtrlHandler(NULL, FALSE);
    }

    if (!processCreated) {
        std::cout << "[TPCShell] Failed to execute command. Windows error: "
                  << createError << std::endl;
        delete[] lpCommandLine;
        return;
    }

    // XỬ LÝ CHẾ ĐỘ FOREGROUND / BACKGROUND THEO ĐỀ BÀI
    if (!isBackground) {
        // 1. Chế độ Foreground: TPCShell đứng đợi tiến trình con chạy xong
        const bool foregroundRegistered =
            setForegroundProcess(pi.dwProcessId, pi.hProcess);
        const DWORD waitResult = WaitForSingleObject(pi.hProcess, INFINITE);

        if (waitResult == WAIT_FAILED) {
            const DWORD errorCode = GetLastError();
            std::cerr << "[TPCShell] Warning: failed to wait for "
                      << "foreground PID " << pi.dwProcessId
                      << " to exit. Windows error: " << errorCode << '\n';
        }

        if (foregroundRegistered) {
            clearForegroundProcess();
        }
        
        // Giải phóng tài nguyên ngay sau khi tiến trình kết thúc
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } 
    else {
        // 2. Chế độ Background: Không chặn Shell, tiến trình chạy ngầm song song
        std::cout << "[TPCShell] Background process started successfully. PID: "
                  << pi.dwProcessId << "\n";
        
        // Đẩy PID, Handle và tên lệnh sang cho Chính nạp vào danh sách quản lý
        addBackgroundProcess(pi.dwProcessId, pi.hProcess, cmdName);
        
        // Đóng handle luồng, giữ lại handle tiến trình (hProcess) để Chính có thể Kill/Stop sau này
        CloseHandle(pi.hThread);
    }

    // Giải phóng bộ nhớ đệm
    delete[] lpCommandLine;
}

CommandResult executeShellLine(const std::string& input) {
    const std::string trimmed = trim(input);
    if (trimmed.empty()) {
        return CommandResult::Continue;
    }

    ParsedCommand parsed = parseCommand(trimmed);
    if (parsed.command.empty()) {
        return CommandResult::Continue;
    }

    if (isBuiltinCommand(parsed.command)) {
        if (parsed.command == "exit") {
            return CommandResult::Exit;
        }

        executeBuiltin(parsed);
        return CommandResult::Continue;
    }

    std::string batchPath;
    if (resolveBatchFile(parsed.command, batchPath)) {
        if (parsed.isBackground) {
            std::cerr << "[TPCShell] Batch files cannot be started with '&' "
                      << "because TPCShell interprets them internally.\n";
            return CommandResult::Continue;
        }

        executeBatchFile(batchPath.c_str());
        return CommandResult::Continue;
    }

    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(parsed.command.c_str()));

    for (const auto& arg : parsed.args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }

    argv.push_back(nullptr);

    executeCommand(argv[0], argv.data(), parsed.isBackground);
    return CommandResult::Continue;
}

void executeBatchFile(const char* filePath) {
    std::ifstream file(filePath);
    
    if (!file.is_open()) {
        std::cout << "[TPCShell] Failed to open script file '" << filePath << "'\n";
        return;
    }

    std::cout << "[TPCShell] Starting batch file execution: " << filePath << "\n";
    beginBatchExecution();

    std::string line;
    
    while (std::getline(file, line)) {
        if (isBatchCancellationRequested()) {
            std::cout << "[TPCShell] Batch execution cancelled by Ctrl+C.\n";
            break;
        }

        const std::string trimmed = trim(line);
        // Bỏ qua dòng trống hoặc comment
        if (isCommentLine(trimmed)) {
            continue;
        }

        std::cout << "-> Run: " << trimmed << "\n";

        if (executeShellLine(trimmed) == CommandResult::Exit) {
            std::cout << "[TPCShell] Batch execution stopped by exit command.\n";
            break;
        }
    }

    file.close();
    endBatchExecution();
    std::cout << "[TPCShell] Finished batch file execution.\n";
}
