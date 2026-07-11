#pragma once
#include <array>
#include <string>
#include <vector>

using ProcessGroup = std::pair<std::string, std::array<long, 2>>;
using ProcessGroupVec = std::vector<ProcessGroup>;

struct Process {
  std::string name;
  long mb;
  int pid;
  std::string exe_path;
};

struct AppGroup {
  std::string name;
  long total_mb;
  int num_processes;
};

// Trimming function of a processs name to remove parenthesis or as a fallback later
// EX: Process names can show up as Code Helper (Renderer) or Obsidian Helper (GPU), etc.
std::string trim_process_name(const std::string &name);

// Get the app name from the full file path of a running process executable.
// This solves the issue of having two processes in the UI such as Claude and
// Claude Helper EX of path:
// /Users/name/Applications/Obsidian.app/Contents/MacOS/Obsidian Note: Need to
// get the first occurance of a app bundle, to get the overal application bundle
// Sometimes there can contain another app bundle within the larger app itself.
std::string app_name_from_exe_path(const std::string &path);

// Returns a vector of processes sorted
// Excludes processes which are protected, so they cannot be deleted.
std::vector<Process> scan_processes(const std::vector<std::string> &protected_processes);

// Returns the group name for a process (used for grouping and killing)
// The root app of all the underlying processes.
std::string group_name_for_process(const Process &process);

// Groups processes by their app name and sums their memory
// usage and counts the number of processes in each group.
// Returns a vector of pairs where the first element is the app name and the
// second element is an array [total_mb, num_processes], sorted by total_mb in
// descending order.
ProcessGroupVec group_processes(const std::vector<Process> &processes);

// A function that takes in the vector of groups processes
// and searches for any processes that begins with the substring
// RETURNS: vector of pair<std::string, std::array<long, 2>>
// in sorted order such that the earlier the search string appears
// (the index of position), the earlier in the vector it will be.
ProcessGroupVec search_processes(std::string &search, const ProcessGroupVec &groups);
