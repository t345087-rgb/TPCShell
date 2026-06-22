#include "parser.h"
#include <sstream>
#include <algorithm>

ParsedCommand parseCommand(const std::string& input) {
    ParsedCommand result;
    result.isBackground = false;
    
    std::string trimmed = input;
    
    // Trim leading/trailing whitespace
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
    
    if (trimmed.empty()) {
        return result;
    }
    
    // Check for background mode (& at the end)
    if (!trimmed.empty() && trimmed.back() == '&') {
        result.isBackground = true;
        trimmed.pop_back();  // Remove the '&'
        // Trim again after removing '&'
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
    }
    
    if (trimmed.empty()) {
        return result;
    }
    
    // Parse command and arguments using stringstream
    std::istringstream iss(trimmed);
    std::string token;
    
    if (iss >> result.command) {
        while (iss >> token) {
            result.args.push_back(token);
        }
    }
    
    return result;
}