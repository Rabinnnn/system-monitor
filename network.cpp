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
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) { // Check if the interface has an IPv4 address (AF_NET) and has an active IP assigned to it (ifa-> ifa_addr)
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

// Function that reads network receive (RX) statistics from /proc/net/dev and
// returns them in a map<string, RX>, where each key is a network interface name (e.g "eth0" or "wlan0")
// and each value is an RX struct holding the stats.
// The purpose of the function is to parse and extract network receive statistics (e.g number of bytes and packets)
// for each network interface.
map<string, RX> NetworkTracker::getNetworkRX() {
    map<string, RX> rxStats;
    ifstream netDevFile("/proc/net/dev");
    string line;
    
    getline(netDevFile, line); // Skip header line
    getline(netDevFile, line); // Skip second header line

    while (getline(netDevFile, line)) {
        istringstream iss(line);
        string interfaceName;
        getline(iss, interfaceName, ':'); // read characters from iss(the line) up to the colon : and stores them in interfaceName
        interfaceName = interfaceName.substr(interfaceName.find_first_not_of(" \t")); // remove any leading spaces or tabs from the interface name
        
        RX rx;
        // extract numbers from the rest of the line
        iss >> rx.bytes >> rx.packets >> rx.errs >> rx.drop 
            >> rx.fifo >> rx.frame >> rx.compressed >> rx.multicast;
        
        rxStats[interfaceName] = rx; // store the parsed stats into the map using the interface name as the key
    }

    netDevFile.close();
    return rxStats;
}

// Function that reads the file /proc/net/dev to extract transmit (TX) statistics
// for each network interface and returns them as a map
map<string, TX> NetworkTracker::getNetworkTX() {
    map<string, TX> txStats;
    ifstream netDevFile("/proc/net/dev");
    string line;
    
    getline(netDevFile, line); // Skip header line
    getline(netDevFile, line); // Skip second header line

    while (getline(netDevFile, line)) {
        istringstream iss(line);
        string interfaceName;
        getline(iss, interfaceName, ':');
        interfaceName = interfaceName.substr(interfaceName.find_first_not_of(" \t"));
        
        long long rxDummy;
        for (int i = 0; i < 8; ++i) { // TX stats appear after the first 8 RX values. In this loop, We read and discard the RX values
            iss >> rxDummy;
        }
        
        TX tx;
        iss >> tx.bytes >> tx.packets >> tx.errs >> tx.drop 
            >> tx.fifo >> tx.colls >> tx.carrier >> tx.compressed;
        
        txStats[interfaceName] = tx;
    }

    netDevFile.close();
    return txStats;
}