#include "process.h"
#include "kill_utils.h"
#include <signal.h>
#include <string>
#include <vector>

// Kills all processes in the given group name (base name, as used in group_processes)
void kill_process_group(const std::string &group_name, const std::vector<Process> &processes) {
    for (const auto &p : processes) {
        // Clean process name to match group_processes logic
        std::string base_name = p.name;
        size_t paren = base_name.find('(');
        if (paren != std::string::npos) {
            base_name = base_name.substr(0, paren);
        }
        while (!base_name.empty() && base_name.back() == ' '){
            base_name.pop_back();
        }
        if (base_name == group_name){
            kill(p.pid, SIGKILL);
        }
    }
}
