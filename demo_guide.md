# TPCShell Demo Guide

A step-by-step tutorial to demonstrate all features of TPCShell for grading evaluation.

---

## 1. Running the Shell

**Compile and Run:**
```bash
# Using Visual Studio (MSVC)
cl /EHsc /W4 main.cpp process_mgr.cpp controller.cpp parser.cpp builtins.cpp
TPCShell.exe

# Using MinGW
g++ -o TPCShell main.cpp process_mgr.cpp controller.cpp parser.cpp builtins.cpp -lwinapi
TPCShell.exe
```

**Output:**
```
======================================
  Chao mung den voi TPCShell (v1.0)   
======================================
Type 'help' for available commands.

TPCShell> _
```

---

## 2. Built-in Commands (Nội trú - 2đ)

### `help` - Display all commands
```
TPCShell> help

=== TPCShell Help ===
Available commands:

Built-in Commands:
  help              - Show this help message
  exit              - Exit the shell
  date              - Display current date
  time              - Display current time
  dir [path]        - List directory contents
  cd <path>         - Change directory
  path              - Display PATH environment variable
  addpath <dir>     - Add directory to PATH
  delpath <dir>     - Remove directory from PATH

Process Commands:
  list              - List background processes
  kill <PID>        - Terminate a background process
  stop <PID>        - Pause a background process
  resume <PID>      - Resume a paused process
========================
```

### `date` - Display current date
```
TPCShell> date
Current date: Sunday, June 22, 2026
```

### `time` - Display current time
```
TPCShell> time
Current time: 16:45:30
```

### `dir` - List directory contents
```
TPCShell> dir
TPCShell> dir C:\Windows
```

### `cd` - Change directory
```
TPCShell> cd C:\Users
TPCShell> cd ..
TPCShell> cd .
```

---

## 3. Environment Variables (Môi trường - 3đ)

### `path` - Display PATH variable
```
TPCShell> path

PATH environment variable:
  1. C:\Windows
  2. C:\Windows\System32
  3. C:\Program Files
```

### `addpath` - Add directory to PATH
```
TPCShell> addpath C:\MyPrograms
Added 'C:\MyPrograms' to PATH
```

### `delpath` - Remove directory from PATH
```
TPCShell> delpath C:\MyPrograms
Removed 'C:\MyPrograms' from PATH
```

---

## 4. External Program Execution (Foreground - 2đ)

Run a blocking foreground process:
```
TPCShell> notepad.exe
[TPCShell waits until notepad is closed]
```

---

## 5. Background Processes (Background - 2đ)

Run process in background with `&`:
```
TPCShell> notepad.exe &
[TPCShell] Tiến trình chay ngam kich hoat thanh cong. PID: 1234
TPCShell> _
```

Multiple background processes:
```
TPCShell> mspaint.exe &
[TPCShell] Tiến trình chay ngam kich hoat thanh cong. PID: 5678
TPCShell> calc.exe &
[TPCShell] Tiến trình chay ngam kich hoat thanh cong. PID: 9012
```

---

## 6. Process Management (List/Stop/Kill - 4đ)

### `list` - Show all background processes
```
TPCShell> list

Background Processes:
  PID: 1234  |  notepad.exe  |  Running
  PID: 5678  |  mspaint.exe   |  Running
  PID: 9012  |  calc.exe      |  Running
```

### `stop <PID>` - Pause a process
```
TPCShell> stop 1234
[TPCShell] Da tam dung tien trinh PID: 1234
```

Verify stopped:
```
TPCShell> list

Background Processes:
  PID: 1234  |  notepad.exe  |  Stopped
  PID: 5678  |  mspaint.exe  |  Running
  PID: 9012  |  calc.exe     |  Running
```

### `resume <PID>` - Resume paused process
```
TPCShell> resume 1234
[TPCShell] Da tiep tuc tien trinh PID: 1234
```

### `kill <PID>` - Terminate a process
```
TPCShell> kill 1234
[TPCShell] Da kill tien trinh PID: 1234
```

Verify killed:
```
TPCShell> list

Background Processes:
  PID: 5678  |  mspaint.exe  |  Running
  PID: 9012  |  calc.exe     |  Running
```

---

## 7. Batch File Execution (File .bat - 3đ)

### `test.bat` contains:
```
calc.exe
mspaint.exe
```

### Run batch file:
```
TPCShell> test.bat
[TPCShell] Bat dau thuc thi file script: test.bat
-> Run: calc.exe
-> Run: mspaint.exe
[TPCShell] Hoan thanh thuc thi file script.
```

---

## 8. Signal Handling (Ctrl+C - 2đ)

Run a long-running foreground process:
```
TPCShell> ping localhost -t
Pinging localhost with 32 bytes of data:
Reply from ::1: time<1ms
Reply from ::1: time<1ms
...
```

Press `Ctrl+C` to terminate:
```
^C
[TPCShell] Received Ctrl+C - terminating child process
TPCShell> _
```

The shell remains running after Ctrl+C.

---

## 9. Exit Shell

```
TPCShell> exit
Goodbye!
```

---

## Demo Checklist Summary

| Feature | Points | Command(s) | Demo Step |
|---------|--------|------------|-----------|
| Nội trú (help, dir, cd, time) | 2đ | `help`, `date`, `time`, `dir`, `cd` | Section 2 |
| Background processes | 2đ | `command &` | Section 5 |
| List/Stop/Kill/Resume | 4đ | `list`, `stop`, `kill`, `resume` | Section 6 |
| Foreground execution | 2đ | `notepad.exe` | Section 4 |
| Ctrl+C handling | 2đ | `ping -t` + Ctrl+C | Section 8 |
| Environment (path/addpath/delpath) | 3đ | `path`, `addpath`, `delpath` | Section 3 |
| Batch file (.bat) | 3đ | `test.bat` | Section 7 |
| Report | 2đ | Documentation | README + demo_guide |

**Total: 20 points**

---

## Tips for Demo

1. **Start with `help`** - Shows all available commands
2. **Sequence matters**:
   - Start shell → `help`
   - Built-in commands → `date`, `time`, `dir`, `cd`
   - Environment → `path`, `addpath`, `delpath`
   - Foreground → Run `notepad`
   - Background → `notepad &`
   - Process control → `list`, `stop`, `kill`
   - Signal → `ping -t` + Ctrl+C
   - Batch → `test.bat`
3. **Explain each step** as you demonstrate
4. **Handle errors** - Try `cd invalid_path` to show error handling
5. **Demo background cleanup** - Kill all background processes before finishing