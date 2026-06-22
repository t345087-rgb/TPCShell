#ifndef BUILTINS_H
#define BUILTINS_H

#include <string>
#include <vector>
#include "parser.h"

// Built-in commands
void cmdHelp();
void cmdDate();
void cmdTime();
void cmdDir(const std::vector<std::string>& args);
void cmdCd(const std::vector<std::string>& args);
void cmdPath();
void cmdAddPath(const std::string& dir);
void cmdDelPath(const std::string& dir);

// Kiểm tra xem lệnh có phải là built-in không
bool isBuiltinCommand(const std::string& cmd);

// Thực thi built-in command
void executeBuiltin(const ParsedCommand& parsed);

#endif