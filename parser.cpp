#include "parser.h"
#include <sstream>
#include <algorithm>

namespace {
void trimInPlace(std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t");
    if (first == std::string::npos) {
        value.clear();
        return;
    }

    const std::size_t last = value.find_last_not_of(" \t");
    value = value.substr(first, last - first + 1);
}
}

ParsedCommand parseCommand(const std::string& input) {
    ParsedCommand result;
    result.isBackground = false;
    
    std::string trimmed = input;
    
    // Trim leading/trailing whitespace
    trimInPlace(trimmed);
    
    if (trimmed.empty()) {
        return result;
    }
    
    // Check for background mode (& at the end)
    if (!trimmed.empty() && trimmed.back() == '&') {
        result.isBackground = true;
        trimmed.pop_back();  // Remove the '&'
        // Trim again after removing '&'
        trimInPlace(trimmed);
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
