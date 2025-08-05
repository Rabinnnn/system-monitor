#include "header.h"
#include <cstring>
#include <fstream>
#include <pwd.h>
#include <sstream>
#include <unistd.h>

#include <fstream>
#include <string>
#include <sstream>

// get cpu id and information, you can use `proc/cpuinfo`
string CPUinfo()
{
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0, 0, 0, 0};

    // unix system
    // for windoes maybe we must add the following
    // __cpuid(regs, 0);
    // regs is the array of 4 positions
    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);

        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    string str(CPUBrandString);
    return str;
}

// getOsName, this will get the OS of the current computer
const char *getOsName()
{
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

string getCurrentUsername() {
    uid_t uid = getuid(); // get the user id
    struct passwd *pw = getpwuid(uid); 
    if (pw) return pw->pw_name; // if the uid returned a password struct extract the username
    const char* user = getenv("USER"); // if getpwuid failed, try to get the username from the environment
    if (user) return user;
    return "Unknown";
}

// get the name of the host machine
string getHostname() {
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) == 0) { //use sizeof(hostname) to avoid buffer overflow
        hostname[HOST_NAME_MAX-1] = '\0'; // ensure null termination
        return string(hostname);
    }
    return "Unknown";
}

// countProcessStates returns a map of process states e.g Running, sleeping, 
// zombie etc and their counts. It gets the info from /proc which is a directory
// containin process info
map<char, int> countProcessStates() {
    map<char, int> processStates;
    DIR *dir = opendir("/proc"); 
    if (!dir) return processStates;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) { // iterate over each entry in the /proc directory
        // Check if the directory name is numeric (PID)
        if (isdigit(entry->d_name[0])) {
            string statPath = "/proc/" + string(entry->d_name) + "/stat";
            ifstream statFile(statPath); // open file using input file stream
            if (statFile.is_open()) {
                string fullStat;
                getline(statFile, fullStat); //read entire line of the file
                size_t statePos = fullStat.find_last_of(")");
                if (statePos != string::npos && statePos + 2 < fullStat.length()) {
                    char state = fullStat[statePos + 2]; // extract the process state character
                    // Map 'I' (idle) to 'S' (sleeping) to match top's behavior
                    if (state == 'I') state = 'S';
                    processStates[state]++;
                }
                statFile.close();
            }
        }
    }
    closedir(dir);
    return processStates;
}  

// returns the total number of processes found
int getTotalProcessCount() {
    map<char, int> states = countProcessStates();
    int total = 0;
    for (const auto& pair : states) total += pair.second; // pair.first is the process state e.g 'R' while pair.second is the number of processes
    return total;
}



// getCPUTemperature retrieves the current CPU temp on a Linux system using several
// different methods. If one method fails it tries a different one. It is designed to
// be compatible with different hardware vendors and kernel configurations.
float getCPUTemperature() {
    // Method 1: Try to find coretemp in hwmon devices. hwmon dir contains hardware monitor devices.
    DIR* hwmonDir = opendir("/sys/class/hwmon"); 
    if (hwmonDir) {
        struct dirent* entry;
        while ((entry = readdir(hwmonDir)) != nullptr) {
            if (entry->d_type == DT_LNK || entry->d_type == DT_DIR) { //ensure only link directories are read
                if (strncmp(entry->d_name, "hwmon", 5) == 0) {
                    std::string namePath = "/sys/class/hwmon/" + std::string(entry->d_name) + "/name";
                    std::ifstream nameFile(namePath);
                    std::string name;
                    if (nameFile.is_open() && std::getline(nameFile, name)) {
                        if (name == "coretemp") {
                            // Found coretemp, read Package id 0 temperature
                            std::string tempPath = "/sys/class/hwmon/" + std::string(entry->d_name) + "/temp1_input";
                            std::ifstream tempFile(tempPath);
                            int temp;
                            if (tempFile.is_open() && (tempFile >> temp)) {
                                closedir(hwmonDir);
                                return temp / 1000.0f; // Convert from millidegrees to degrees celsius
                            }
                        }
                    }
                }
            }
        }
        closedir(hwmonDir);
    }


    // Method 2: Try x86_pkg_temp thermal zone
    std::ifstream pkgTempFile("/sys/class/thermal/thermal_zone14/temp");
    if (pkgTempFile.is_open()) {
        int temp;
        if (pkgTempFile >> temp) {
            return temp / 1000.0f; // Convert from millidegrees to degrees
        }
    }

      // Method 3: Try to find any CPU-related thermal zone
    DIR* thermalDir = opendir("/sys/class/thermal");
    if (thermalDir) {
        struct dirent* entry;
        while ((entry = readdir(thermalDir)) != nullptr) {
            if (strncmp(entry->d_name, "thermal_zone", 12) == 0) {
                std::string typePath = "/sys/class/thermal/" + std::string(entry->d_name) + "/type"; //check the sensor type
                std::ifstream typeFile(typePath);
                std::string type;
                if (typeFile.is_open() && std::getline(typeFile, type)) {
                    // Look for CPU-related thermal zones
                    if (type.find("x86") != std::string::npos || //npos represents not found
                        type.find("cpu") != std::string::npos ||
                        type.find("CPU") != std::string::npos ||
                        type.find("processor") != std::string::npos) {
                        std::string tempPath = "/sys/class/thermal/" + std::string(entry->d_name) + "/temp";
                        std::ifstream tempFile(tempPath);
                        int temp;
                        if (tempFile.is_open() && (tempFile >> temp)) {
                            closedir(thermalDir);
                            return temp / 1000.0f; // Convert from millidegrees to degrees
                        }
                    }
                }
            }
        }
        closedir(thermalDir);
    }

     // Method 4: ThinkPad-specific method (fallback)
    std::ifstream thinkpadTempFile("/proc/acpi/ibm/thermal");
    if (thinkpadTempFile.is_open()) {
        std::string line;
        if (getline(thinkpadTempFile, line)) {
            // Parse the temperatures line
            // Format is "temperatures: 50 -128 0 0 39 0 0 -128"
            if (line.find("temperatures:") != std::string::npos) {
                std::istringstream iss(line.substr(line.find(":") + 1));
                int temp;
                iss >> temp; // Read the first temperature value
                return temp; // Already in degrees C, no need to divide by 1000
            }
        }
    }

    // default value to be returned if all methods fail
    return 0.0f;

}

float getFanSpeed() {
    // Method 1: Try to find fan speed in hwmon devices
    DIR* hwmonDir = opendir("/sys/class/hwmon");
    if (hwmonDir) {
        struct dirent* entry;
        while ((entry = readdir(hwmonDir)) != nullptr) {
            if (entry->d_type == DT_LNK || entry->d_type == DT_DIR) {
                if (strncmp(entry->d_name, "hwmon", 5) == 0) {
                    // Check for fan1_input or similar files
                    std::string fanPath = "/sys/class/hwmon/" + std::string(entry->d_name) + "/fan1_input";
                    std::ifstream fanFile(fanPath);
                    int speed;
                    if (fanFile.is_open() && (fanFile >> speed)) {
                        closedir(hwmonDir);
                        return static_cast<float>(speed);
                    }
                }
            }
        }
        closedir(hwmonDir);
    }

    // Method 2: Try HP-specific fan information (To Do)
    // HP laptops might have fan information in different locations
    // Research the specific paths for HP EliteBook

    // Method 3: ThinkPad-specific method (fallback)
    std::ifstream thinkpadFanFile("/proc/acpi/ibm/fan");
    if (thinkpadFanFile.is_open()) {
        std::string line;
        while (getline(thinkpadFanFile, line)) {
            // Look for the line that starts with "speed:"
            // Format is "speed:      2814"
            if (line.find("speed:") != std::string::npos) {
                std::istringstream iss(line.substr(line.find(":") + 1));
                float speed;
                iss >> speed;
                return speed;
            }
        }
    }

    // If all methods fail, return 0
    return 0.0f;
}