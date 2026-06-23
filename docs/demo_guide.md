# TPCShell Demo Guide

A concise walkthrough for demonstrating TPCShell features.

## 1. Build And Run

```bash
g++ -std=c++17 -Wall -Wextra main.cpp parser.cpp builtins.cpp process_mgr.cpp controller.cpp -o TPCShell.exe
TPCShell.exe
```

## 2. Built-In Commands

Start with:

```text
TPCShell> help
```

Confirm that help lists `mkdir`, `deldir`, and `killall`.

Directory demo:

```text
TPCShell> mkdir demo_folder
[TPCShell] Directory created: demo_folder
TPCShell> dir
TPCShell> deldir demo_folder
[TPCShell] Directory removed: demo_folder
```

Also show:

```text
TPCShell> date
TPCShell> time
TPCShell> pwd
TPCShell> cd ..
TPCShell> cd TPCShell
TPCShell> clear
```

## 3. TPCShell-Local PATH Demo

`path`, `addpath`, and `delpath` manage a TPCShell-local PATH list. They do not print the full Windows PATH and do not modify the system PATH. TPCShell uses this local list when looking up external commands.

Prepare a demo executable from PowerShell:

```powershell
mkdir C:\TestDemo
Copy-Item C:\Windows\System32\mspaint.exe C:\TestDemo\paintdemo.exe
```

```text
TPCShell> path
[TPCShell] Shell PATH is empty.
TPCShell> addpath C:\TestDemo
[TPCShell] Added PATH entry: C:\TestDemo
TPCShell> path
[TPCShell] Shell PATH entries:
  1. C:\TestDemo
TPCShell> paintdemo.exe &
[TPCShell] Background process started successfully. PID: 1234
TPCShell> list
TPCShell> kill 1234
TPCShell> delpath C:\TestDemo
[TPCShell] Removed PATH entry: C:\TestDemo
TPCShell> path
[TPCShell] Shell PATH is empty.
```

Use the PID shown by your own `list` output.

## 4. Foreground And Ctrl+C

Use a predictable console process for foreground execution:

```text
TPCShell> ping 127.0.0.1 -n 6
```

For Ctrl+C handling:

```text
TPCShell> ping 127.0.0.1 -n 30
```

Press `Ctrl+C`. The foreground command should stop and TPCShell should remain running.

Do not rely on `notepad.exe` or `calc.exe` for foreground/background process-control demos. Some Windows apps may reuse an existing process or return immediately.

## 5. Background Processes

Recommended background GUI:

```text
TPCShell> mspaint.exe &
[TPCShell] Background process started successfully. PID: 1234
```

Recommended background console process:

```text
TPCShell> cmd.exe /c ping 127.0.0.1 -n 30 > NUL &
[TPCShell] Background process started successfully. PID: 5678
```

## 6. Process Management

```text
TPCShell> list
TPCShell> stop 1234
TPCShell> list
TPCShell> resume 1234
TPCShell> kill 1234
TPCShell> list
```

Optional `killall` demo:

```text
TPCShell> mspaint.exe &
[TPCShell] Background process started successfully. PID: 2222
TPCShell> mspaint.exe &
[TPCShell] Background process started successfully. PID: 3333
TPCShell> list
TPCShell> killall
[TPCShell] Terminated 2 background process(es).
TPCShell> list
[TPCShell] No managed background processes.
```

## 7. Batch File

```text
TPCShell> test.bat
```

Batch execution should parse each command and run built-ins or external commands as usual.

## 8. Exit

```text
TPCShell> exit
Goodbye!
```
