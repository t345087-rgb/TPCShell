# TPCShell Demo Routine

A step-by-step demonstration routine covering all scoring criteria.

**Scoring Summary:**
| Criteria | Points | Section |
|----------|--------|---------|
| Nội trú (help, dir, cd, pwd, time, ...) | 2đ | Section 2 |
| Background processes | 2đ | Section 5 |
| List, stop, kill, resume | 4đ | Section 6 |
| Foreground execution | 2đ | Section 4 |
| Ctrl+C signal handling | 2đ | Section 7 |
| Environment (path, addpath, delpath) | 3đ | Section 3 |
| File .bat batch execution | 3đ | Section 8 |
| Report | 2đ | Documentation |

**Total: 20đ**

---

## Section 1: Start Shell

**Command:**
```bash
g++ -o TPCShell main.cpp process_mgr.cpp controller.cpp parser.cpp builtins.cpp
./TPCShell
```

**Expected Output:**
```
======================================
  Welcome to TPCShell (v1.0)          
======================================
Type 'help' for available commands.

TPCShell>
```

---

## Section 2: Nội trú (2đ)

Commands: `help`, `date`, `time`, `dir`, `cd`, `pwd`, `clear`

**Step 1: help**
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
  pwd               - Display current working directory
  clear             - Clear the screen
  path              - Display PATH environment variable
  addpath <dir>     - Add directory to PATH
  delpath <dir>     - Remove directory from PATH

Process Commands:
  list              - List background processes
  stop <PID>        - Pause a background process
  resume <PID>      - Resume a paused process
  kill <PID>        - Terminate a background process
=======================
```

**Step 2: date**
```
TPCShell> date
Current date: Sunday, June 22, 2026
```

**Step 3: time**
```
TPCShell> time
Current time: 16:45:30
```

**Step 4: dir (current directory)**
```
TPCShell> dir
```

**Step 5: pwd**
```
TPCShell> pwd
Current directory: /home/username/TPCShell
```

**Step 6: cd**
```
TPCShell> cd ..
TPCShell> pwd
Current directory: /home/username
TPCShell> cd TPCShell
TPCShell> pwd
Current directory: /home/username/TPCShell
```

**Step 7: clear**
```
TPCShell> clear
```
(Screen clears)

---

## Section 3: Môi trường (3đ)

Commands: `path`, `addpath`, `delpath`

**Step 1: path**
```
TPCShell> path

PATH environment variable:
  1. /usr/local/bin
  2. /usr/bin
  3. /bin
```

**Step 2: addpath**
```
TPCShell> addpath /home/username/programs
Added '/home/username/programs' to PATH

TPCShell> path
PATH environment variable:
  1. /usr/local/bin
  2. /usr/bin
  3. /bin
  4. /home/username/programs
```

**Step 3: delpath**
```
TPCShell> delpath /home/username/programs
Removed '/home/username/programs' from PATH

TPCShell> path
PATH environment variable:
  1. /usr/local/bin
  2. /usr/bin
  3. /bin
```

---

## Section 4: Foreground Execution (2đ)

Command: `notepad.exe`

**Step 1: Run foreground process**
```
TPCShell> notepad.exe
```
(Notepad window opens, shell waits)

**Step 2: Close notepad manually**
(Shell returns to prompt)
```
TPCShell>
```

---

## Section 5: Background Processes (2đ)

Commands: `command &`

**Step 1: Start background process**
```
TPCShell> notepad.exe &
[TPCShell] Background process started successfully. PID: 1234
TPCShell>
```

**Step 2: Start second background process**
```
TPCShell> mspaint.exe &
[TPCShell] Background process started successfully. PID: 5678
TPCShell>
```

---

## Section 6: List, Stop, Kill, Resume (4đ)

Commands: `list`, `stop`, `resume`, `kill`

**Step 1: list (show all background processes)**
```
TPCShell> list

PID         NAME                    STATUS
1234        notepad.exe             RUNNING
5678        mspaint.exe             RUNNING
```

**Step 2: stop (pause a process)**
```
TPCShell> stop 1234
Process PID 1234 stopped successfully.
```

**Step 3: Verify stop**
```
TPCShell> list

PID         NAME                    STATUS
1234        notepad.exe             STOPPED
5678        mspaint.exe             RUNNING
```

**Step 4: resume (resume paused process)**
```
TPCShell> resume 1234
Process PID 1234 resumed successfully.
```

**Step 5: kill (terminate a process)**
```
TPCShell> kill 5678
[TPCShell] Terminated process PID 5678.
```

**Step 6: Verify kill**
```
TPCShell> list

PID         NAME                    STATUS
1234        notepad.exe             RUNNING
```

**Step 7: Clean up remaining process**
```
TPCShell> kill 1234
[TPCShell] Terminated process PID 1234.

TPCShell> list

No background processes running.
```

---

## Section 7: Ctrl+C Signal Handling (2đ)

Command: `ping localhost -t` + Ctrl+C

**Step 1: Start long-running foreground process**
```
TPCShell> ping localhost -t
PING localhost (::1) 56(84) bytes of data.
64 bytes from localhost: icmp_seq=1 ttl=64 time=0.023 ms
64 bytes from localhost: icmp_seq=2 ttl=64 time=0.041 ms
64 bytes from localhost: icmp_seq=3 ttl=64 time=0.038 ms
```

**Step 2: Press Ctrl+C**
```
^C
TPCShell>
```
(Shell remains running, returns to prompt)

---

## Section 8: File .bat Batch Execution (3đ)

**Note:** Ensure `test.bat` contains:
```
notepad.exe
mspaint.exe
```

**Run batch file:**
```
TPCShell> test.bat
[TPCShell] Starting batch file execution: test.bat
-> Run: notepad.exe
-> Run: mspaint.exe
[TPCShell] Finished batch file execution.
```

---

## Section 9: Exit Shell (2đ Report points - manual)

```
TPCShell> exit
Goodbye!
```

---

## Demo Checklist Summary

| Step | Criteria | Points | Command | Done |
|------|----------|--------|---------|------|
| 1 | Start shell | - | `./TPCShell` | ☐ |
| 2.1 | help | - | `help` | ☐ |
| 2.2 | date | - | `date` | ☐ |
| 2.3 | time | - | `time` | ☐ |
| 2.4 | dir | - | `dir` | ☐ |
| 2.5 | pwd | - | `pwd` | ☐ |
| 2.6 | cd | - | `cd ..` | ☐ |
| 2.7 | clear | - | `clear` | ☐ |
| 3.1 | path | - | `path` | ☐ |
| 3.2 | addpath | - | `addpath <dir>` | ☐ |
| 3.3 | delpath | - | `delpath <dir>` | ☐ |
| 4 | Foreground | 2đ | `notepad.exe` | ☐ |
| 5 | Background | 2đ | `notepad.exe &` | ☐ |
| 6.1 | list | - | `list` | ☐ |
| 6.2 | stop | - | `stop <PID>` | ☐ |
| 6.3 | resume | - | `resume <PID>` | ☐ |
| 6.4 | kill | - | `kill <PID>` | ☐ |
| 7 | Ctrl+C | 2đ | `ping -t` + Ctrl+C | ☐ |
| 8 | Batch .bat | 3đ | `test.bat` | ☐ |
| 9 | Exit | - | `exit` | ☐ |

**Total Points Demonstrated: 9đ** (plus 2đ for Report, 3đ Environment = 14đ shown, 6đ for other demos)

---

## Quick Demo Sequence (Full Run)

```
./TPCShell
help
date
time
dir
pwd
cd ..
pwd
cd TPCShell
path
addpath /tmp
path
delpath /tmp
path
notepad.exe
(notepad opens, close it manually)
notepad.exe &
mspaint.exe &
list
stop 1234
list
resume 1234
kill 5678
list
kill 1234
list
ping localhost -t
(press Ctrl+C)
test.bat
exit