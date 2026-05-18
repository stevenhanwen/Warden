#include "process.h"
#include <libproc.h>
#include <algorithm>

// Scans all processes and returns a vector of Process structs with their name, memory usage in MB, and PID
// protected_processes is a list of process names that should be ignored/skipped in the scan 
std::vector<Process> scan_processes(const std::vector<std::string>& protected_processes) {
    std::vector<Process> processes;
    int pids[1024];
    int count = proc_listallpids(pids, sizeof(pids));

    for (int i = 0; i < count; i++) {
        struct proc_taskinfo info;
        char name[64];

        proc_name(pids[i], name, sizeof(name));
        int res = proc_pidinfo(pids[i], PROC_PIDTASKINFO, 0, &info, sizeof(info));

        // res is the number of bytes filled in the buffer/info struct if successful.
        if (res > 0) {
            long mb = info.pti_resident_size / 1024 / 1024;
            if (mb > 0) {
                // check if protected
                bool is_protected = false;
                for (auto& p : protected_processes) {
                    if (p == name) {
                        is_protected = true;
                        break;
                    }
                }
                if (!is_protected) {
                    processes.push_back({name, mb, pids[i]});
                }
            }
        }
    }

    std::sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.mb > b.mb;
    });

    return processes;
}

// Groups processes by their base name (removing any parenthesis and trailing spaces) and sums their memory 
// usage and counts the number of processes in each group. 
// Returns a vector of pairs where the first element is the app name and the second element is an array 
// [total_mb, num_processes], sorted by total_mb in descending order.
std::vector<std::pair<std::string, std::array<long, 2>>> group_processes(const std::vector<Process>& processes) {
    std::map<std::string, std::array<long, 2>> app_groups_map;

    // Let value be an array [a, b] where a is the total mb and b is the num_processes
    // Clean each process name and check if it is in the map
    for (auto& p : processes) {
        // Search for a parenthesis if there is one
        size_t paren = p.name.find('(');
        std::string base_name;

        if (paren == std::string::npos) {
            base_name = p.name; // no ( found, use full name
        } else {
            base_name = p.name.substr(0, paren); // cut at (
        }

        // trim trailing space
        while (!base_name.empty() && base_name.back() == ' ') {
            base_name.pop_back();
        }

        if (app_groups_map.find(base_name) != app_groups_map.end()) {
            app_groups_map[base_name][0] += p.mb;
            app_groups_map[base_name][1] += 1;
        } else {
            app_groups_map.insert({base_name, {p.mb, 1}});
        }
    }

    // convert to a vector to sort by total mb for each app group.
    // Use pair data structure to hold the key and its value that is an array 
    std::vector<std::pair<std::string, std::array<long, 2>>> apps_vector(app_groups_map.begin(), app_groups_map.end());
    std::sort(apps_vector.begin(), apps_vector.end(), [](const auto& a, const auto& b) {
        return a.second[0] > b.second[0];
    });

    return apps_vector;
}