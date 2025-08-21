#include "header.h"
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <dirent.h>
#include <cctype>
#include <cmath> // Include for std::round

// Function that reads Linux system memory stats from /proc/meminfo and converts them
// into a MemoryInfo struct.
MemoryInfo SystemResourceTracker::getMemoryInfo() {
    MemoryInfo mem = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    // Use a unified map to store all values
    std::map<std::string, unsigned long> memStats;
    std::ifstream meminfo("/proc/meminfo");
    std::string line;

    while (std::getline(meminfo, line)) {
        std::istringstream iss(line);
        std::string key;
        unsigned long value;
        iss >> key >> value;
        // The colon is part of the key in /proc/meminfo
        if (!key.empty()) {
            key.pop_back(); // Remove the colon
            memStats[key] = value;
        }
    }
    meminfo.close();

    unsigned long memTotal = memStats["MemTotal"];
    unsigned long memAvailable = memStats["MemAvailable"];
    unsigned long swapTotal = memStats["SwapTotal"];
    unsigned long swapFree = memStats["SwapFree"];

    // Calculate used memory based on MemAvailable, matching 'free' command logic
    unsigned long usedMem = memTotal - memAvailable;

    // Convert from KB to GiB (1 GiB = 1024^3 bytes)
    mem.total_ram = static_cast<float>(memTotal) / (1024.0f * 1024.0f); 
    mem.used_ram = static_cast<float>(usedMem) / (1024.0f * 1024.0f);   

    // Round the total_ram value to match 'free -h' output
    mem.total_ram = std::round(mem.total_ram);

    mem.ram_percent = (mem.total_ram > 0) ? (mem.used_ram / mem.total_ram * 100.0f) : 0.0f;

    mem.total_swap = static_cast<float>(swapTotal) / (1024.0f * 1024.0f);
    mem.used_swap = static_cast<float>(swapTotal - swapFree) / (1024.0f * 1024.0f);
    mem.swap_percent = (mem.total_swap > 0) ? (mem.used_swap / mem.total_swap * 100.0f) : 0.0f;

    return mem;
}

// This is a method of the SystemResourceTracer class.
// It returns  disk information struct containing total_space, used_space, usage_percent
DiskInfo SystemResourceTracker::getDiskInfo() {
    struct statvfs stat;
    statvfs("/", &stat);

    DiskInfo disk;
    disk.total_space = static_cast<float>(stat.f_blocks) * stat.f_frsize / (1024.0f * 1024.0f * 1024.0f);
    disk.used_space = static_cast<float>(stat.f_blocks - stat.f_bfree) * stat.f_frsize / (1024.0f * 1024.0f * 1024.0f);

    // Round off the values
    disk.total_space = std::round(disk.total_space);
    disk.used_space = std::round(disk.used_space);

    disk.usage_percent = (disk.total_space > 0) ? (disk.used_space / disk.total_space * 100.0f) : 0.0f;

    return disk;
}

// Function that reads and returns a list of all running processes on a Linux system
std::vector<Proc> SystemResourceTracker::getProcessList() {
    std::vector<Proc> processes;
    DIR *dir = opendir("/proc");
    if (!dir) return processes;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        // check if entry is a directory and its name starts with a digit
        if (entry->d_type == DT_DIR && std::isdigit(entry->d_name[0])) {
            std::string pid = entry->d_name;
            std::string statPath = "/proc/" + pid + "/stat";
            std::ifstream statFile(statPath);

            if (statFile.is_open()) {
                Proc process;
                process.pid = std::stoi(pid); //parse pid string to an integer

                std::string line;
                std::getline(statFile, line);

                size_t nameStart = line.find('(');
                size_t nameEnd = line.rfind(')');

                // extracts the process name between the parentheses
                if (nameStart != std::string::npos && nameEnd != std::string::npos) {
                    process.name = line.substr(nameStart + 1, nameEnd - nameStart - 1);

                    // create an input string stream from the remainder of the line after the closing parenthesis
                    std::istringstream iss(line.substr(nameEnd + 1));
                    std::string field; // variable for temporarily holding each extracted word/token
                    std::vector<std::string> fields;

                    while (iss >> field) {
                        fields.push_back(field);
                    }

                    if (fields.size() >= 24) {
                        process.state = fields[0][0];
                        process.vsize = std::stoll(fields[20]); //virtual memory size
                        process.rss = std::stoll(fields[21]); // resident set size
                        process.utime = std::stoll(fields[11]); // user mode CPU time
                        process.stime = std::stoll(fields[12]); // kernel mode CPU time
                    }

                    processes.push_back(process);
                }
                statFile.close();
            }
        }
    }

    closedir(dir);
    return processes;
}