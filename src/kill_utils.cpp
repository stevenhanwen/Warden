#include "process.h"
#include "kill_utils.h"
#include <signal.h>
#include <string>
#include <vector>

// Kills all processes in the given group name (base name, as used in group_processes)
void kill_process_group(const std::string &group_name, const std::vector<Process> &processes)
{
    for (const auto &p : processes)
    {
        if (group_name_for_process(p) == group_name)
        {
            kill(p.pid, SIGKILL);
        }
    }
}
