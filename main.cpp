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


static CPUUsageTracker cpuTracker;
static ProcessUsageTracker processTracker;
static vector<float> cpuUsageHistory(100, 0.0f);
static vector<float> temperatureHistory(100, 0.0f);
static NetworkRate rateTracker;
static vector<float> cpuUsageBuffer(5, 0.0f);  // Buffer for last 5 readings
static int bufferIndex = 0;

// Timing variables for graph updates
static float cpuUpdateTime = 0.0f;
static float fanUpdateTime = 0.0f;
static float thermalUpdateTime = 0.0f;

// system monitoring UI function with tabs for CPU, Fan, and Thermal info, plus system metadata.
// id is unique identifier for the window, size refers to the window size in pixels, while position
// refers to window position on the screen.
void systemWindow(const char* id, ImVec2 size, ImVec2 position) {
    ImGuiIO& io = ImGui::GetIO(); // get reference to ImGui's IO interface
    ImGui::Begin(id);
    ImGui::SetWindowSize(size);
    ImGui::SetWindowPos(position);

    ImGui::BeginChild("SystemInfo", ImVec2(0, 150), true); // create a child window(scrollable sub-section)
    ImGui::Text("Operating System: %s", getOsName());
    ImGui::Text("Username: %s", getCurrentUsername().c_str());
    ImGui::Text("Hostname: %s", getHostname().c_str());
    ImGui::Text("Total Processes: %d", getTotalProcessCount());
    ImGui::Text("CPU Type: %s", CPUinfo().c_str());

    static auto lastUpdateTime = std::chrono::steady_clock::now(); // last time process states were updated
    static std::map<char, int> cachedStates; // count of processes per state (e.g Running, Sleeping etc)
    
    //update process state counts only once per second
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdateTime).count() >= 1) {
        cachedStates = countProcessStates();
        lastUpdateTime = now;
    }
    
    ImGui::Text("Process States:");
    // Define known states with their labels
    const vector<pair<char, string>> stateLabels = {
        {'R', "Running"},
        {'S', "Sleeping"},
        {'D', "Uninterruptible Sleep"},
        {'Z', "Zombie"},
        {'T', "Stopped"},
        {'I', "Idle"}
    };
    
    int totalProcesses = 0;
    for (const auto& [state, count] : cachedStates) {
        totalProcesses += count;
    }
    ImGui::Text("  Total Processes: %d", totalProcesses);

    // Display known states first
    for (const auto& [code, label] : stateLabels) {
        if (cachedStates.count(code)) {
            ImGui::Text("  %s: %d", label.c_str(), cachedStates.at(code));
        }
    }
    
    // Display any unknown states
    for (const auto& [state, count] : cachedStates) {
        bool isKnown = any_of(stateLabels.begin(), stateLabels.end(),
                            [state](const auto& pair) { return pair.first == state; });
        if (!isKnown) {
            ImGui::Text("  Unknown State (%c): %d", state, count);
        }
    }
    ImGui::EndChild();

    // start tab for CPU, Fan, and Thermal
    if (ImGui::BeginTabBar("SystemPerformanceTabs")) {
       if (ImGui::BeginTabItem("CPU")) {
        static bool pauseGraph = false;
        static float graphFPS = 30.0f;
        static float graphYScale = 100.0f;
        float currentCPUUsage = cpuTracker.calculateCPUUsage();

        // Add moving average calculation
        cpuUsageBuffer[bufferIndex] = currentCPUUsage;
        bufferIndex = (bufferIndex + 1) % cpuUsageBuffer.size();

        // calculate average CPU usage over last N readings
        float smoothedCPUUsage = 0.0f;
        for (float usage : cpuUsageBuffer) {
            smoothedCPUUsage += usage;
        }
        smoothedCPUUsage /= cpuUsageBuffer.size();

        if (!pauseGraph) {
            float updateInterval = 1.0f / graphFPS;
            cpuUpdateTime += io.DeltaTime;
            if (cpuUpdateTime >= updateInterval) {
                cpuUsageHistory.erase(cpuUsageHistory.begin());
                cpuUsageHistory.push_back(smoothedCPUUsage);  // Use smoothed value
                cpuUpdateTime = 0.0f;
            }
        }

        ImGui::Checkbox("Pause Graph", &pauseGraph);
        ImGui::SliderFloat("Graph FPS", &graphFPS, 1.0f, 60.0f);
        ImGui::SliderFloat("Y-Scale", &graphYScale, 10.0f, 200.0f);

        ImGui::PlotLines("CPU Usage", cpuUsageHistory.data(), cpuUsageHistory.size(),
                        0, TextF("CPU: %.1f%%", smoothedCPUUsage).c_str(),  // Use smoothed value
                        0.0f, graphYScale, ImVec2(0, 80));
        ImGui::EndTabItem();
    }
}