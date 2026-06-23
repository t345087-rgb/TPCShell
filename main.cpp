
#include <iostream>
#include <string>
#include <vector>
#include "process_mgr.h"
#include "controller.h"
#include "parser.h"
#include "builtins.h"

int main(int argc, char* argv[]) {
    setupSignalHandler();

    // Batch mode: chạy script khi gọi TPCShell.exe <file>
    // Ví dụ: TPCShell.exe test.bat
    if (argc >= 2) {
        executeBatchFile(argv[1]);
        return 0;
    }

    std::string input;
    
    std::cout << "======================================\n";
    std::cout << "  Welcome to TPCShell (v1.0)          \n";
    std::cout << "======================================\n";
    std::cout << "Type 'help' for available commands.\n\n";

    while (true) {
        std::cout << "TPCShell> ";

        if (!std::getline(std::cin, input)) {
            std::cout << "\nGoodbye!\n";
            break;
        }
        
        // Trim whitespace
        std::string trimmed = input;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));

        std::size_t last = trimmed.find_last_not_of(" \t");
        if (last == std::string::npos) {
            continue;
        }
        trimmed.erase(last + 1);
        
        if (trimmed.empty()) {
            continue;
        }
        
        // Parse command
        ParsedCommand parsed = parseCommand(trimmed);
        
        if (parsed.command.empty()) {
            continue;
        }
        
        // Check for built-in commands
        if (isBuiltinCommand(parsed.command)) {
            if (parsed.command == "exit") {
                std::cout << "Goodbye!\n";
                break;
            }

            executeBuiltin(parsed);
            continue;
        }
        
        // External command: prepare argv for executeCommand
        std::vector<char*> commandArgv;
        commandArgv.push_back(const_cast<char*>(parsed.command.c_str()));

        for (const auto& arg : parsed.args) {
            commandArgv.push_back(const_cast<char*>(arg.c_str()));
        }

        commandArgv.push_back(nullptr);
        
        // Execute external command
        executeCommand(commandArgv[0], commandArgv.data(), parsed.isBackground);
    }

    return 0;
}

