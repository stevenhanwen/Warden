#include <iostream>
#include <libproc.h>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <map>
#include <array>


struct Process {
    std::string name;
    long mb;
    int pid;
};

struct AppGroup {
    std::string group_name; 
    long total_mb;
    int num_processes;
};

int main(int argc, char *argv[]) {
    long limit = 500; // default limit of 500mb
    bool watch_mode = false;
    bool kill_mode = false;

    // read config file
    std::ifstream config("warden.conf");
    std::vector<std::string> protected_processes;
    if (config.is_open()) {
        std::string line;
        while (std::getline(config, line))
        {
            // searches for "limit=" starting at position 0
            // rfind returns the position in which it was found,
            // which will be 0, otherwise returns npos
            if (line.rfind("limit=", 0) == 0)
            {
                limit = atol(line.substr(6).c_str());
            }
            if (line.rfind("protect=", 0) == 0)
            {
                protected_processes.push_back(line.substr(8));
            }
        }
    }

    if (argc > 1) {
        for (int i = 1; i < argc; i++)
        {
            if (std::string(argv[i]) == "--watch")
            {
                watch_mode = true;
            }
            else if (atol(argv[i]) > 0)
            {
                // use the limit argument if provided and convert to a long
                limit = atol(argv[1]);
            }
            else if (std::string(argv[i]) == "--kill")
            {
                kill_mode = true;
            }
        }
    }

    // temporarily can't use watch and kill together
    if (watch_mode && kill_mode)
    {
        std::cout << "error: cannot use --watch and --kill together\n";
        return 1;
    }

    do {
        if (watch_mode) {
            system("clear");
            std::cout << "Warden — watching processes (limit: " << limit << "mb)\n";
            std::cout << "─────────────────────────────────────────\n";
        }

        int pids[1024];
        int count = proc_listallpids(pids, sizeof(pids));
        std::vector<Process> processes;

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

        // sort by memory highest first
        std::sort(processes.begin(), 
                  processes.end(), 
                  [](const Process& a, const Process& b) {return a.mb > b.mb;});


        // Group processes by a app groups and search using a map
        std::map<std::string, std::array<long, 2>> app_groups_map;
        // Let value be an array [a, b] where a is the total mb and b is the num_processes
        // Clean each process name and check if it is in the map
        for (auto& p : processes) {
            // Search for a parenthesis if there is one
            size_t paren = p.name.find('(');
            std::string base_name;

            if (paren == std::string::npos) {
                base_name = p.name;  // no ( found, use full name
            } else {
                base_name = p.name.substr(0, paren);  // cut at (
            }

            // trim trailing space
            while (!base_name.empty() && base_name.back() == ' ') {
                base_name.pop_back();
            }

            if (app_groups_map.find(base_name) != app_groups_map.end()) {
                app_groups_map[base_name][0] += p.mb;
                app_groups_map[base_name][1] += 1;
            }
            else {
                app_groups_map.insert({base_name, {p.mb, 1}});
            }
        }

        // TODO: convert to a vector to sort by total mb for each app group. 


        // print sorted processes
        // int counter = 0;
        for (const auto& app_values : app_groups_map) {
            const std::string& app_name = app_values.first;
            const std::array<long, 2>& values = app_values.second;

            if (values[0] > limit) {
                std::cout << "⚠️  " << app_name << " is using lots of memory: " << values[0] << " MB (" << values[1] << "processes)" << "\n";
                if (kill_mode) {
                    std::cout << "Are you sure you want to kill " << app_name << "? (y/n): ";
                    char confirm;
                    std::cin >> confirm;
                    if (confirm == 'y') {
                        std::cout << "🔪 killing " << app_name<< "\n";
                        // TO DO:
                        // Modify this to use a for loop to kill the processes: kill(p.pid, SIGTERM);
                    } else {
                        std::cout << "skipping " << app_name << "\n";
                    }
                }
            } else {
                std::cout << "✅ " << app_name << " — " << values[0] << " MB (" << values[1] << "processes)" << "\n";
            }

            // if (counter == 20) {
            //     break; 
            // }

            // counter ++; 
        }

        if (watch_mode) {
            std::cout << "─────────────────────────────────────────\n";
            std::cout << "updating every 3 seconds. press Ctrl+C to stop.\n";
            sleep(3);
        }

    } while (watch_mode);
}