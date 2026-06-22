#include <iostream>
#include <string>
#include <vector>
#include "process_mgr.h"
#include "controller.h"
#include "parser.h"
#include "builtins.h"

int main() {
    setupSignalHandler();
    std::string input;
    
    std::cout << "======================================\n";
    std::cout << "  Chao mung den voi TPCShell (v1.0)   \n";
    std::cout << "======================================\n";
    std::cout << "Type 'help' for available commands.\n\n";

    while (true) {
        std::cout << "TPCShell> ";
        std::getline(std::cin, input);
        
        // Trim whitespace
        std::string trimmed = input;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
        
        if (trimmed.empty()) continue;
        
        // Parse command
        ParsedCommand parsed = parseCommand(trimmed);
        
        if (parsed.command.empty()) continue;
        
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
        // Convert vector<string> to array of char*
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(parsed.command.c_str()));
        for (const auto& arg : parsed.args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        // Execute external command
        executeCommand(argv[0], argv.data(), parsed.isBackground);
    }
    return 0;
}
