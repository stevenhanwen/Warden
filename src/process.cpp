#include "process.h"
#include <algorithm>
#include <libproc.h>
#include <map>
#include <unordered_map>

// Trimming function of a processs name to remove parenthesis or as a fallback
// later EX: Process names can show up as Code Helper (Renderer) or Obsidian
// Helper (GPU), etc.
static std::string trim_process_name(const std::string &name) {
  size_t paren = name.find('(');
  std::string base_name = (paren == std::string::npos) ? name : name.substr(0, paren);
  while (!base_name.empty() && base_name.back() == ' ') {
    base_name.pop_back();
  }
  return base_name;
}

// Get the app name from the full file path of a running process executable.
// This solves the issue of having two processes in the UI such as Claude and
// Claude Helper EX of path:
// /Users/name/Applications/Obsidian.app/Contents/MacOS/Obsidian Note: Need to
// get the first occurance of a app bundle, to get the overal application bundle
// Sometimes there can contain another app bundle within the larger app itself.
static std::string app_name_from_exe_path(const std::string &path) {
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

ProcessGroup group_processes(const std::vector<Process> &processes) {
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
  std::vector<std::pair<std::string, std::array<long, 2>>> apps_vector(app_groups_map.begin(),
                                                                       app_groups_map.end());
  std::sort(apps_vector.begin(), apps_vector.end(),
            [](const auto &a, const auto &b) { return a.second[0] > b.second[0]; });

  return apps_vector;
}

ProcessGroup
search_processes(std::string &search,
                 const std::vector<std::pair<std::string, std::array<long, 2>>> &groups) {
  std::vector<std::pair<std::string, std::array<long, 2>>> result;

  // Use a unordered_map to quickly search the group name and see its position index
  std::unordered_map<std::string, int> group_search_map;

  for (const auto &group : groups) {
    size_t position = group.first.find(search);
    if (position == std::string::npos) {
      continue;
    }

    group_search_map[group.first] = position;

    // Just insert the group if result is empty
    if (result.empty()) {
      result.push_back(group);
    } else if (position >= group_search_map[result.at(result.size() - 1).first]) {
      // Push back to the end if it is greatest
      result.push_back(group);
    }

    // Need to place the current group in the correct order.
    // Move through the result vector to place the group based on start
    // of where the substring is found inside the group name.
    // Essentially a insertion sort algorithm.
    for (size_t i = 0; i < result.size(); ++i) {
      size_t curr_pos = group_search_map[result.at(i).first];
      if (position <= curr_pos) {
        result.insert(result.begin() + i, group);
        break;
      }
    }
  }

  return result;
}
