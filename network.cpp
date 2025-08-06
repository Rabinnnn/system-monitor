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

// Function that retrieves IPV4 network interfaces
Networks NetworkTracker::getNetworkInterfaces() {
    Networks nets;
    struct ifaddrs *ifap, *ifa; // ifap holds the start of the linked list
                                // of interfaces while ifa is an iterator for traversing the list.
    
    if (getifaddrs(&ifap) == -1) { // getifaddrs populates ifap with a linked list of network interfaces. If it fails return the empty nets
        return nets;
    }

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) { // Check if the interface has an IPv4 address
            struct sockaddr_in *addr = (struct sockaddr_in*)ifa->ifa_addr;
            
            // extract the IPv4 address and name
            IP4 interface;
            interface.name = strdup(ifa->ifa_name);
            inet_ntop(AF_INET, &(addr->sin_addr), interface.addressBuffer, INET_ADDRSTRLEN);
            
            nets.ip4s.push_back(interface); // add the interface to the list of IPv4 interfaces
        }
    }

    freeifaddrs(ifap); // free the memory allocated by getifaddrs
    return nets;
}