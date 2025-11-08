#include <ncurses.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <pwd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

struct ProcInfo {
    int pid;
    uid_t uid;
    std::string user;
    std::string cmd;
    unsigned long utime = 0;
    unsigned long stime = 0;
    long rss = 0;
    unsigned long vsize = 0;
    double cpu_percent = 0.0;
    double mem_percent = 0.0;
};

std::string uid_to_user(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    if (pw) return pw->pw_name;
    return std::to_string(uid);
}

unsigned long long read_total_jiffies() {
    std::ifstream f("/proc/stat");
    std::string cpu;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    f >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

unsigned long long read_mem_total_kb() {
    std::ifstream f("/proc/meminfo");
    std::string key;
    unsigned long long value;
    std::string unit;
    while (f >> key >> value >> unit) {
        if (key == "MemTotal:") return value;
    }
    return 0;
}

bool is_number(const std::string &s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

std::vector<int> list_pids() {
    std::vector<int> pids;
    DIR *dir = opendir("/proc");
    if (!dir) return pids;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name(entry->d_name);
        if (is_number(name)) pids.push_back(std::stoi(name));
    }
    closedir(dir);
    return pids;
}

ProcInfo read_proc(int pid) {
    ProcInfo p;
    p.pid = pid;
    std::ifstream statf("/proc/" + std::to_string(pid) + "/stat");
    if (statf) {
        std::string line;
        std::getline(statf, line);
        std::istringstream iss(line);
        std::vector<std::string> toks;
        std::string token;
        while (iss >> token) toks.push_back(token);
        if (toks.size() > 23) {
            p.utime = std::stoul(toks[13]);
            p.stime = std::stoul(toks[14]);
            p.vsize = std::stoul(toks[22]);
            p.rss = std::stol(toks[23]);
            std::string comm = toks[1];
            if (comm.front() == '(' && comm.back() == ')')
                comm = comm.substr(1, comm.size() - 2);
            p.cmd = comm;
        }
    }

    std::ifstream status("/proc/" + std::to_string(pid) + "/status");
    if (status) {
        std::string line;
        while (std::getline(status, line)) {
            if (line.rfind("Uid:", 0) == 0) {
                std::istringstream iss(line.substr(4));
                iss >> p.uid;
                p.user = uid_to_user(p.uid);
            }
        }
    }
    if (p.user.empty()) p.user = uid_to_user(p.uid);
    return p;
}

int main() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    int highlight = 0;
    int interval_s = 2;
    enum SortMode {BY_CPU, BY_MEM, BY_PID};
    SortMode sortmode = BY_CPU;

    long page_size = sysconf(_SC_PAGESIZE);
    unsigned long long prev_total_jiffies = read_total_jiffies();
    std::map<int, unsigned long> prev_proc_jiffies;

    while (true) {
        unsigned long long total_jiffies = read_total_jiffies();
        unsigned long long mem_total_kb = read_mem_total_kb();

        std::vector<ProcInfo> procs;
        auto pids = list_pids();
        for (int pid : pids) {
            ProcInfo pi = read_proc(pid);
            unsigned long cur = pi.utime + pi.stime;
            unsigned long prev = prev_proc_jiffies.count(pid) ? prev_proc_jiffies[pid] : 0;
            unsigned long delta = (cur >= prev) ? (cur - prev) : 0;
            unsigned long total_delta = (total_jiffies >= prev_total_jiffies)
                                            ? (total_jiffies - prev_total_jiffies)
                                            : 0;
            double cpu_pct = (total_delta > 0) ? (100.0 * delta / total_delta) : 0.0;
            double mem_pct = (mem_total_kb > 0)
                                 ? (100.0 * ((pi.rss * page_size) / 1024.0) / mem_total_kb)
                                 : 0.0;
            pi.cpu_percent = cpu_pct;
            pi.mem_percent = mem_pct;
            procs.push_back(pi);
            prev_proc_jiffies[pid] = cur;
        }
        prev_total_jiffies = total_jiffies;

        if (sortmode == BY_CPU)
            std::sort(procs.begin(), procs.end(), [](auto &a, auto &b) {
                return a.cpu_percent > b.cpu_percent;
            });
        else if (sortmode == BY_MEM)
            std::sort(procs.begin(), procs.end(), [](auto &a, auto &b) {
                return a.mem_percent > b.mem_percent;
            });
        else
            std::sort(procs.begin(), procs.end(), [](auto &a, auto &b) {
                return a.pid < b.pid;
            });

        clear();
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        mvprintw(0, 0, "SimpleSysMon  (q quit, c=CPU, m=MEM, p=PID, k=kill, +/- interval=%ds)", interval_s);
        mvprintw(1, 0, "Total jiffies: %llu  MemTotal: %llu kB  Procs: %zu", total_jiffies, mem_total_kb, procs.size());
        mvprintw(3, 0, "PID   USER       %%CPU   %%MEM    VSZ      RSS(KB)  CMD");

        int max_display = rows - 5;
        for (int i = 0; i < max_display && i < (int)procs.size(); ++i) {
            const auto &p = procs[i];
            if (i == highlight) attron(A_REVERSE);
            mvprintw(4 + i, 0, "%-5d %-10s %6.2f  %6.2f  %-8lu %-8ld  %.40s",
                     p.pid, p.user.c_str(), p.cpu_percent, p.mem_percent,
                     p.vsize, (long)(p.rss * (page_size / 1024)), p.cmd.c_str());
            if (i == highlight) attroff(A_REVERSE);
        }

        refresh();

        int ch = getch();
        if (ch != ERR) {
            if (ch == 'q') break;
            if (ch == KEY_UP) highlight = std::max(0, highlight - 1);
            if (ch == KEY_DOWN) highlight = std::min((int)procs.size() - 1, highlight + 1);
            if (ch == 'c') sortmode = BY_CPU;
            if (ch == 'm') sortmode = BY_MEM;
            if (ch == 'p') sortmode = BY_PID;
            if (ch == '+') interval_s = std::max(1, interval_s - 1);
            if (ch == '-') interval_s += 1;
            if (ch == 'k' && highlight >= 0 && highlight < (int)procs.size()) {
                int pid = procs[highlight].pid;
                int res = kill(pid, SIGTERM);
                mvprintw(2, 0, (res == 0) ? "Killed PID %d" : "Failed to kill PID %d", pid);
                refresh();
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(interval_s));
    }

    endwin();
    return 0;
}
