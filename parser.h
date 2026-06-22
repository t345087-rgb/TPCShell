#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

struct ParsedCommand {
    std::string command;      // Tên lệnh
    std::vector<std::string> args;  // Danh sách tham số
    bool isBackground;        // Chế độ chạy ngầm (&)
};

ParsedCommand parseCommand(const std::string& input);

#endif