#include "header.h"
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <algorithm>

// Function that reads Linux system memory stats from /proc/meminfo and converts them
// into a MemoryInfo struct.
MemoryInfo SystemResourceTracker::getMemoryInfo() {
    MemoryInfo mem = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    // declare temporary variables that will hold raw values (in kB) read from /proc/meminfo
    unsigned long memTotal = 0, memFree = 0;
    unsigned long buffers = 0, cached = 0, sReclaimable = 0;
    unsigned long swapTotal = 0, swapFree = 0;

    std::ifstream meminfo("/proc/meminfo"); //open file for reading
    std::string line;
    while (std::getline(meminfo, line)) { // read one line at a time into line until EOF
        std::istringstream iss(line);
        std::string key;
        unsigned long value;
        //split into key value pairs, extract the details assign the parsed value into corresponding temporary variable
        iss >> key >> value;
        if (key == "MemTotal:") memTotal = value;
        else if (key == "MemFree:") memFree = value;
        else if (key == "Buffers:") buffers = value;
        else if (key == "Cached:") cached = value;
        else if (key == "SReclaimable:") sReclaimable = value;
        else if (key == "SwapTotal:") swapTotal = value;
        else if (key == "SwapFree:") swapFree = value;
    }
    meminfo.close();

    // Calculate used memory as shown by 'free -h'
    unsigned long usedMem = memTotal - memFree - buffers - (cached + sReclaimable);

    mem.total_ram = memTotal / (1024.0f * 1024.0f); // KB to GB
    mem.used_ram = usedMem / (1024.0f * 1024.0f);   // KB to GB
    mem.ram_percent = (mem.total_ram > 0) ? (mem.used_ram / mem.total_ram * 100.0f) : 0.0f;

    mem.total_swap = swapTotal / (1024.0f * 1024.0f);
    mem.used_swap = (swapTotal - swapFree) / (1024.0f * 1024.0f);
    mem.swap_percent = (mem.total_swap > 0) ? (mem.used_swap / mem.total_swap * 100.0f) : 0.0f;

    return mem;
}

// This is a method of the SystemResourceTracer class.
// It returns  disk information struct containing total_space, used_space, usage_percent
DiskInfo SystemResourceTracker::getDiskInfo() {
    struct statvfs stat;
    statvfs("/", &stat);

    DiskInfo disk;
    disk.total_space = stat.f_blocks * stat.f_frsize / (1024 * 1024 * 1024);
    disk.used_space = (stat.f_blocks - stat.f_bfree) * stat.f_frsize / (1024 * 1024 * 1024);
    disk.usage_percent = (float)disk.used_space / disk.total_space * 100.0f;

    return disk;
}