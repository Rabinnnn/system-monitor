// To make sure you don't declare the function more than once by including the header multiple times.
#ifndef header_H
#define header_H

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <dirent.h>
#include <vector>
#include <iostream>
#include <cmath>
// lib to read from file
#include <fstream>
// for the name of the computer and the logged in user
#include <unistd.h>
#include <limits.h>
// this is for us to get the cpu information
// mostly in unix system
// not sure if it will work in windows
#include <cpuid.h>
// this is for the memory usage and other memory visualization
// for linux gotta find a way for windows
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
// for time and date
#include <ctime>
// ifconfig ip addresses
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

using namespace std;

struct CPUStats
{
    long long int user;
    long long int nice;
    long long int system;
    long long int idle;
    long long int iowait;
    long long int irq;
    long long int softirq;
    long long int steal;
    long long int guest;
    long long int guestNice;
};

// processes `stat`
struct Proc
{
    int pid;
    string name;
    char state;
    long long int vsize;
    long long int rss;
    long long int utime;
    long long int stime;
};

struct IP4
{
    char *name;
    char addressBuffer[INET_ADDRSTRLEN];
};

struct Networks
{
    vector<IP4> ip4s;
    ~Networks();

};

struct TX
{
    int bytes;
    int packets;
    int errs;
    int drop;
    int fifo;
    int frame;
    int compressed;
    int multicast;
};

struct RX
{
    int bytes;
    int packets;
    int errs;
    int drop;
    int fifo;
    int colls;
    int carrier;
    int compressed;
};

// student TODO : system stats
string CPUinfo();
const char *getOsName();

// student TODO : memory and processes

// student TODO : network


struct MemoryInfo {
    float total_ram;   // Total RAM in GB
    float used_ram;    // Used RAM in GB
    float ram_percent; // Percentage of RAM used
    float total_swap;  // Total swap in GB
    float used_swap;   // Used swap in GB
    float swap_percent; // Percentage of swap used
};


struct DiskInfo {
    float total_space;   // Total disk space in GB
    float used_space;    // Used disk space in GB
    float usage_percent; // Percentage of disk used
};

class SystemResourceTracker {
public:
    MemoryInfo getMemoryInfo();
    DiskInfo getDiskInfo();
    vector<Proc> getProcessList();
};

class CPUUsageTracker {
private:
    CPUStats lastStats;
    float currentUsage;

public:
    CPUUsageTracker();
    float calculateCPUUsage();
    float getCurrentUsage();
};

class ProcessUsageTracker {
    private:
        map<int, pair<long long, long long>> lastProcessCPUTime;
        float deltaTime;
        float updateInterval;
        float lastUpdateTime;
        map<int, float> cpuUsageCache;
    
    public:
        ProcessUsageTracker();
        float calculateProcessCPUUsage(const Proc& process, float currentTime); // No change needed, just context
        void updateDeltaTime(float dt);
    };

class NetworkTracker {
public:
    Networks getNetworkInterfaces();
    map<string, RX> getNetworkRX();
    map<string, TX> getNetworkTX();
};

// System functions
string CPUinfo();
const char* getOsName();
string getCurrentUsername();
string getHostname();
map<char, int> countProcessStates();
int getTotalProcessCount();
float getCPUTemperature();
float getFanSpeed();
string formatNetworkBytes(long long bytes);

template<typename... Args>
string TextF(const char* fmt, Args... args) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), fmt, args...);
    return string(buffer);
}

#endif
