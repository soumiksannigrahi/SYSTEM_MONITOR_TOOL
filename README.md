# 🖥️ System Monitor Tool

A console-based system monitor tool built in **C++** that displays real-time information about system processes, memory usage, and CPU load — similar to the Linux `top` command.

Built using **ncurses** for the terminal UI and **Linux `/proc` filesystem** for gathering system data.

---

## 📋 Features

| Day | Feature | Description |
|-----|---------|-------------|
| **Day 1** | UI Layout & System Data | Designed terminal UI using ncurses; gathers system-wide CPU and memory stats from `/proc/stat` and `/proc/meminfo` |
| **Day 2** | Process List Display | Enumerates all running processes from `/proc/[pid]/stat` and displays PID, USER, CPU%, MEM%, VSZ, RSS, and command name |
| **Day 3** | Process Sorting | Sort processes by CPU usage, memory usage, PID, or process name; toggle ascending/descending order |
| **Day 4** | Kill Processes | Select a process using arrow keys and kill it with a confirmation prompt (`y/n`) before termination |
| **Day 5** | Real-Time Refresh | Auto-refreshes the process list every N seconds (adjustable 1–10s); non-blocking input during refresh |

---

## 🛠️ Prerequisites

- **WSL (Windows Subsystem for Linux)** or a native Linux environment
- **g++** compiler (C++17 or later)
- **libncurses-dev** library

### Install dependencies (Ubuntu/Debian):

```bash
sudo apt update
sudo apt install g++ libncurses-dev
```

---

## 🚀 How to Compile & Run

### Step 1: Open WSL Terminal

Open **Windows PowerShell** and type:

```powershell
wsl
```

### Step 2: Navigate to the Project Directory

```bash
cd /mnt/d/Anti_Gravity/'System Monitor Tool'
```

### Step 3: Compile the Source Code

```bash
g++ -o sysmon src/main.cpp -lncurses -lpthread -std=c++17
```

### Step 4: Run the Tool

```bash
./sysmon
```

### One-Liner (from PowerShell directly):

```powershell
wsl -- bash -c "cd /mnt/d/Anti_Gravity/'System Monitor Tool' && g++ -o sysmon src/main.cpp -lncurses -lpthread -std=c++17 && ./sysmon"
```

---

## ⌨️ Keyboard Controls

| Key | Action |
|-----|--------|
| `c` | Sort by CPU% |
| `m` | Sort by Memory% |
| `p` | Sort by PID |
| `n` | Sort by Process Name |
| `r` | Reverse sort order (ASC ↔ DESC) |
| `↑` / `↓` | Navigate / highlight a process |
| `k` | Kill the highlighted process (with y/n confirmation) |
| `+` | Increase refresh interval |
| `-` | Decrease refresh interval |
| `q` | Quit the application |

---

## 📸 Sample Output

```
SimpleSysMon  (q=quit c=CPU m=MEM p=PID n=NAME r=reverse k=kill +/- interval=2s)  Sort: CPU% DESC
Total jiffies: 394516  MemTotal: 7821496 kB  Procs: 23

PID   USER       %CPU   %MEM    VSZ      RSS(KB)  CMD
168   root         0.00    0.02  3235840  1920      agetty
397   soumik       0.00    0.06  7192576  4480      sysmon
359   soumik       0.00    0.06  6217728  4992      bash
335   soumik       0.00    0.04  21659648 3516      (sd-pam)
334   soumik       0.00    0.14  20795392 11136     systemd
291   root         0.00    0.06  6852608  4352      login
290   soumik       0.00    0.07  6217728  5248      bash
289   root         0.00    0.01  3162112  1160      Relay(290)
288   root         0.00    0.01  3162112  1024      SessionLeader
194   root         0.00    0.28  109592576 22016    unattended-upgr
188   root         0.00    0.02  3190784  1792      agetty
177   syslog       0.00    0.07  227848192 5760     rsyslogd
1     root         0.00    0.16  22302720 12356     systemd
166   root         0.00    0.17  1874280448 13056   wsl-pro-service
164   root         0.00    0.11  18399232 8320      systemd-logind
157   messagebus   0.00    0.06  9863168  4992      dbus-daemon
156   root         0.00    0.03  4337664  2432      cron
107   systemd-timesync  0.00  0.10  93212672  7680  systemd-timesyn
106   systemd-resolve   0.00  0.16  21975040 12416  systemd-resolve
90    root         0.00    0.08  25481216 6144      systemd-udevd
40    root         0.00    0.20  43261952 15340     systemd-journal
7     root         0.00    0.02  3145728  1792      init
2     root         0.00    0.02  3145728  1664      init-systemd(Ub
```

---

## 📂 Project Structure

```
System Monitor Tool/
├── src/
│   └── main.cpp        # Main source code (all features)
├── sysmon              # Compiled binary (after compilation)
└── README.md           # This file
```

---

## 🔧 Technical Details

### System Data Sources

| Data | Source | API/Method |
|------|--------|------------|
| Total CPU usage | `/proc/stat` | Reads total jiffies (user, nice, system, idle, etc.) |
| Total Memory | `/proc/meminfo` | Reads `MemTotal` value in kB |
| Process list | `/proc/[pid]/` | Scans numeric directories in `/proc` |
| Per-process CPU | `/proc/[pid]/stat` | Fields 14 (utime) and 15 (stime) in jiffies |
| Per-process Memory | `/proc/[pid]/stat` | Fields 23 (vsize) and 24 (rss) in pages |
| Process owner | `/proc/[pid]/status` | Reads `Uid` field, resolves via `getpwuid()` |

### CPU% Calculation

```
CPU% = (delta_process_jiffies / delta_total_jiffies) × 100
```

Where:
- `delta_process_jiffies` = change in (utime + stime) between two samples
- `delta_total_jiffies` = change in total system jiffies between two samples

---

## ⚠️ Notes

- Run with `sudo` to see CPU% and memory for all system processes
- The tool monitors **Linux processes inside WSL**, not native Windows processes
- Refresh interval is adjustable between **1 to 10 seconds**
- Protected system processes (PID 0, 1) should not be killed

---

## 👤 Author

**Soumik Sannigrahi**
