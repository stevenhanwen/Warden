#include "process.h"
#include <algorithm>
#include <cctype>
#include <libproc.h>
#include <map>
#include <unordered_map>

std::string trim_process_name(const std::string &name) {
  size_t paren = name.find('(');
  std::string base_name = (paren == std::string::npos) ? name : name.substr(0, paren);
  while (!base_name.empty() && base_name.back() == ' ') {
    base_name.pop_back();
  }
  return base_name;
}

std::string app_name_from_exe_path(const std::string &path) {
  // Find the first occurrence of ".app" in the path
  size_t app_pos = path.find(".app");
  while (app_pos != std::string::npos) {
    size_t after = app_pos + 4; // Position just after ".app"
    // Check if ".app" is at the end of the string or followed by a '/'
    if (after == path.size() || path[after] == '/') {
      // Find the last '/' before the ".app" to get the start of the app name
      size_t slash = path.rfind('/', app_pos);
      // Ensure the slash is found and is before ".app"
      if (slash != std::string::npos && slash + 1 < app_pos) {
        // Extract the substring between the slash and ".app" (the app name)
        return path.substr(slash + 1, app_pos - slash - 1);
      }
      break; // If not found, exit the loop
    }
    // Look for the next occurrence of ".app" in the path
    app_pos = path.find(".app", app_pos + 4);
  }
  // Return empty string if no valid app name is found
  return "";
}

std::string group_name_for_process(const Process &process) {
  if (!process.exe_path.empty()) {
    std::string app_name = app_name_from_exe_path(process.exe_path);
    if (!app_name.empty()) {
      return app_name;
    }
  }
  // Fall back to just cleaning the process name if it is not part of a app
  // bundle.
  return trim_process_name(process.name);
}

// Scans all processes and returns a vector of Process structs with their name,
// memory usage in MB, and PID protected_processes is a list of process names
// that should be ignored/skipped in the scan
std::vector<Process> scan_processes(const std::vector<std::string> &protected_processes) {
  std::vector<Process> processes;
  int pids[1024];
  int count = proc_listallpids(pids, sizeof(pids));

  for (int i = 0; i < count; i++) {
    struct proc_taskinfo info;
    char name[64];

    proc_name(pids[i], name, sizeof(name));
    int res = proc_pidinfo(pids[i], PROC_PIDTASKINFO, 0, &info, sizeof(info));

    // res is the number of bytes filled in the buffer/info struct if
    // successful.
    if (res > 0) {
      long mb = info.pti_resident_size / 1024 / 1024;
      if (mb > 0) {
        // check if protected
        bool is_protected = false;
        for (auto &p : protected_processes) {
          if (p == name) {
            is_protected = true;
            break;
          }
        }
        if (!is_protected) {
          // Allocate a fixed-size buffer on the stack,
          // sized to the macOS constant for the maximum process path length.
          char exe_path_buf[PROC_PIDPATHINFO_MAXSIZE];
          int path_len = proc_pidpath(pids[i], exe_path_buf, sizeof(exe_path_buf));

          // Converts buffer to a string if possible
          std::string exe_path = (path_len > 0) ? std::string(exe_path_buf) : "";
          // Append a Process with the process name, memory usage (in MB), PID,
          // and executable path
          processes.push_back({name, mb, pids[i], exe_path});
        }
      }
    }
  }

  std::sort(processes.begin(), processes.end(),
            [](const Process &a, const Process &b) { return a.mb > b.mb; });

  return processes;
}

ProcessGroupVec group_processes(const std::vector<Process> &processes) {
  std::map<std::string, std::array<long, 2>> app_groups_map;

  // Let value be an array [a, b] where a is the total mb and b is the
  // num_processes Clean each process name from its file path using
  // group_name_for_process() and check if it is in the map
  for (auto &p : processes) {
    std::string base_name = group_name_for_process(p);

    if (app_groups_map.find(base_name) != app_groups_map.end()) {
      app_groups_map[base_name][0] += p.mb;
      app_groups_map[base_name][1] += 1;
    } else {
      app_groups_map.insert({base_name, {p.mb, 1}});
    }
  }

  // convert to a vector to sort by total mb for each app group.
  // Use pair data structure to hold the key and its value that is an array
  ProcessGroupVec apps_vector(app_groups_map.begin(), app_groups_map.end());
  std::sort(apps_vector.begin(), apps_vector.end(),
            [](const auto &a, const auto &b) { return a.second[0] > b.second[0]; });

  return apps_vector;
}

ProcessGroupVec search_processes(std::string &search, const ProcessGroupVec &groups) {
  ProcessGroupVec result;

  // Use a unordered_map to quickly search the group name and see its position index
  std::unordered_map<std::string, int> group_search_map;
  // Convert to lowercase for search keyword
  std::transform(search.begin(), search.end(), search.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  for (const auto &group : groups) {
    std::string name = group.first;
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    size_t position = name.find(search);
    if (position == std::string::npos) {
      continue;
    }

    // First push back all the elements, then heapfiy the vector.
    // Should improve time complexity to just O(n).
    group_search_map[group.first] = position;
    result.push_back(group);
  }

  std::make_heap(result.begin(), result.end(),
                 [&group_search_map](const ProcessGroup &a, const ProcessGroup &b) {
                   return group_search_map[a.first] < group_search_map[b.first];
                 });

  return result;
}
