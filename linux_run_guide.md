# TPCShell Linux Demo Guide (Arch Linux + Wine)

**Prerequisites:**
```bash
# Install Wine (if not already installed) and cross-compiler
sudo pacman -S wine mingw-w64-gcc

# Verify Wine installation
wine --version
```

**Compile and Run:**
```bash
# Compile with g++ (no external libs needed)
x86_64-w64-mingw32-g++ -o TPCShell.exe main.cpp process_mgr.cpp controller.cpp parser.cpp builtins.cpp -static

# Run the shell
wine TPCShell.exe
```
