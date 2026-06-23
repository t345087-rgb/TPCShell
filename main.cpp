
#include <iostream>
#include <string>
#include "process_mgr.h"
#include "controller.h"

int main(int argc, char* argv[]) {
    setupSignalHandler();
    bool continuedAfterBatch = false;

    // Batch mode: chạy script khi gọi TPCShell.exe <file>
    // Ví dụ: TPCShell.exe test.bat
    if (argc >= 2) {
        executeBatchFile(argv[1]);
        if (!hasManagedBackgroundProcesses()) {
            return 0;
        }

        continuedAfterBatch = true;
    }

    std::string input;
    
    if (continuedAfterBatch) {
        std::cout << "[TPCShell] Batch completed with managed background "
                  << "processes still running.\n";
        std::cout << "Type 'list' to inspect them or 'help' for commands.\n\n";
    } else {
        std::cout << "======================================\n";
        std::cout << "  Welcome to TPCShell (v1.0)          \n";
        std::cout << "======================================\n";
        std::cout << "Type 'help' for available commands.\n\n";
    }

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
        
        if (executeShellLine(trimmed) == CommandResult::Exit) {
            std::cout << "Goodbye!\n";
            break;
        }
    }

    return 0;
}

