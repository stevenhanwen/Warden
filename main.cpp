#include <iostream>
#include <libproc.h>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <signal.h>

struct Process
{
    std::string name;
    long mb;
    int pid;
};

int main(int argc, char *argv[])
{
    long limit = 500; // default limit of 500mb
    bool watch_mode = false;
    bool kill_mode = false;

    // read config file
    std::ifstream config("warden.conf");
    std::vector<std::string> protected_processes;
    if (config.is_open())
    {
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

    if (argc > 1)
    {
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

        // print sorted processes
        for (auto& p : processes) {
            if (p.mb > limit) {
                std::cout << "⚠️  " << p.name << " is using lots of memory: " << p.mb << " MB\n";
                if (kill_mode) {
                    std::cout << "Are you sure you want to kill " << p.name << "? (y/n): ";
                    char confirm;
                    std::cin >> confirm;
                    if (confirm == 'y') {
                        std::cout << "🔪 killing " << p.name << " (PID: " << p.pid << ")\n";
                        kill(p.pid, SIGTERM);
                    } else {
                        std::cout << "skipping " << p.name << "\n";
                    }
                }
            } else {
                std::cout << "✅ " << p.name << " — " << p.mb << " MB\n";
            }
        }

        if (watch_mode) {
            std::cout << "─────────────────────────────────────────\n";
            std::cout << "updating every 3 seconds. press Ctrl+C to stop.\n";
            sleep(3);
        }

    } while (watch_mode);
}