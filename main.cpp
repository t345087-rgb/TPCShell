#include <iostream>
#include <string>
#include "process_mgr.h"
#include "controller.h"

int main() {
    setupSignalHandler();
    std::string input;
    
    std::cout << "======================================\n";
    std::cout << "  Chao mung den voi TPCShell (v1.0)   \n";
    std::cout << "======================================\n";

    while (true) {
        std::cout << "TPCShell> ";
        std::getline(std::cin, input);
        
        if (input == "exit") break;
        if (input.empty()) continue;

        // TUẤN SẼ LÀM PARSER Ở ĐÂY
        // Tạm thời gọi mẫu thử hàm của Phương:
        executeBatchFile(input.c_str());
    }
    return 0;
}
