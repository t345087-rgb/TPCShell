# TPCShell Demo Routine

Use this runbook during grading. Keep PIDs from your own `list` output.

## 1. Start

```text
TPCShell.exe
help
```

Expected: help lists built-ins, process commands, `mkdir`, `deldir`, and `killall`.

## 2. Built-Ins

```text
date
time
pwd
dir
mkdir demo_folder
dir
deldir demo_folder
cd ..
pwd
cd TPCShell
pwd
clear
```

Expected directory messages:

```text
[TPCShell] Directory created: demo_folder
[TPCShell] Directory removed: demo_folder
```

## 3. TPCShell-Local PATH

This is a shell-local demo list, not the full Windows PATH.

```text
path
addpath C:\TestDemo
path
delpath C:\TestDemo
path
```

Expected key output:

```text
[TPCShell] Shell PATH is empty.
[TPCShell] Added PATH entry: C:\TestDemo
[TPCShell] Shell PATH entries:
  1. C:\TestDemo
[TPCShell] Removed PATH entry: C:\TestDemo
```

## 4. Foreground

Use a reliable foreground console command:

```text
ping 127.0.0.1 -n 6
```

Do not rely on `notepad.exe` or `calc.exe` for process-control demos. Some Windows apps may reuse an existing process or return immediately.

## 5. Background

Recommended background GUI:

```text
mspaint.exe &
```

Recommended background console process:

```text
cmd.exe /c ping 127.0.0.1 -n 30 > NUL &
```

Then run:

```text
list
```

## 6. Stop, Resume, Kill

Replace `<PID>` with a managed PID shown by `list`.

```text
stop <PID>
list
resume <PID>
list
kill <PID>
list
```

## 7. Optional Killall

```text
mspaint.exe &
mspaint.exe &
list
killall
list
```

Expected:

```text
[TPCShell] Terminated 2 background process(es).
[TPCShell] No managed background processes.
```

## 8. Ctrl+C

```text
ping 127.0.0.1 -n 30
```

Press `Ctrl+C`. TPCShell should return to the prompt and keep running.

## 9. Batch

```text
test.bat
```

## 10. Exit

```text
exit
```
