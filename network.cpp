#include "header.h"
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

// Function that converts byte count into a human-readable string
// using appropriate units (B,KB, MB...)
string formatNetworkBytes(long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double value = bytes;

    while (value >= 1024 && unitIndex < 4) {
        value /= 1024;
        unitIndex++;
    }

    char buffer[50];
    // format result to 2 decimal places and the appropriate unit
    snprintf(buffer, sizeof(buffer), "%.2f %s", value, units[unitIndex]);
    return string(buffer);
}