#include "header.h"
#include <SDL2/SDL.h>
#include <GL/gl3w.h>
#include <vector>
#include <algorithm>
#include <set>
#include <chrono>

// A struct that measures and smooths network upload/download rates
// in bytes per second over time
struct NetworkRate {
    map<string, pair<long long, float>> lastRX, lastTX; // Last bytes, timestamp
    map<string, float> rxRate, txRate; // Smoothed rates in bytes/sec
    float lastUpdateTime = 0.0f; // Last time rates were updated
    static constexpr float UPDATE_INTERVAL = 0.5f; // Update every 0.5 seconds
    static constexpr float ALPHA = 0.3f; // Smoothing factor (0 < ALPHA < 1, lower = smoother)

    void update(NetworkTracker& tracker, float time) {
        // Only update rates if enough time has passed
        if (time - lastUpdateTime < UPDATE_INTERVAL) {
            return;
        }
        lastUpdateTime = time;

        auto rxStats = tracker.getNetworkRX();
        auto txStats = tracker.getNetworkTX();

        // Update RX rates
        for (auto& [iface, rx] : rxStats) {
            if (lastRX.count(iface)) {
                float dt = time - lastRX[iface].second; //dt is time since last sample
                float instantaneousRate = dt > 0 ? (rx.bytes - lastRX[iface].first) / dt : 0;
                // Apply exponential moving average
                if (rxRate.count(iface)) {
                    rxRate[iface] = ALPHA * instantaneousRate + (1.0f - ALPHA) * rxRate[iface];
                } else {
                    rxRate[iface] = instantaneousRate; // Initialize with first value
                }
            }
            lastRX[iface] = {rx.bytes, time};
        }

        // Update TX rates
        for (auto& [iface, tx] : txStats) {
            if (lastTX.count(iface)) {
                float dt = time - lastTX[iface].second;
                float instantaneousRate = dt > 0 ? (tx.bytes - lastTX[iface].first) / dt : 0;
                // Apply exponential moving average
                if (txRate.count(iface)) {
                    txRate[iface] = ALPHA * instantaneousRate + (1.0f - ALPHA) * txRate[iface];
                } else {
                    txRate[iface] = instantaneousRate; // Initialize with first value
                }
            }
            lastTX[iface] = {tx.bytes, time};
        }
    }
};