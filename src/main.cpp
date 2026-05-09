#include <iostream>
#include <unistd.h>
#include <signal.h>
#include "config.h"
#include "process.h"
#include <ncurses.h>


int main(int argc, char* argv[]) {
    initscr();
    noecho(); // doesnt print keypresses
    cbreak(); // makes keypresses instant without needing enter
    keypad(stdscr, true); // allows for arrow keys
    timeout(5000); // auto update screen 
    Config config = load_config("warden.conf");
    long limit = config.limit;

    int selected = 0;  // currently selected item

    while (true) {
        clear(); // wipe the canvas

        std::string line(COLS, '-');

        mvprintw(0, 0, "Warden - Select a Process:");
        mvprintw(1, 0, "%s", line.c_str());

        std::vector<Process> processes = scan_processes(config.protected_processes);
        std::vector<std::pair<std::string, std::array<long, 2>>> groups = group_processes(processes);

        int app_index = 0; 
        for (const auto& app_values : groups) {
            const std::string& app_name = app_values.first;
            const std::array<long, 2>& values = app_values.second;

            if (app_index == selected) {
                attron(A_REVERSE);  // highlight selected
                mvprintw(app_index + 2, 0, " -> %s %i MB (%i processes)", app_name.c_str(), values[0], values[1]);
                attroff(A_REVERSE); // turn off highlight
            } else {
                mvprintw(app_index + 2, 0, " -> %s %i MB (%i processes)", app_name.c_str(), values[0], values[1]);
            }
            app_index++; 
        }

        mvprintw(groups.size() + 3, 0, "%s", line.c_str());
        mvprintw(groups.size() + 4, 0, "[up/down] navigate  [q] quit");

        refresh(); // Prints to canvas

        int key = getch();
        if (key == 'q') break;
        if (key == KEY_UP && selected > 0) selected--;
        if (key == KEY_DOWN && selected < (int)groups.size() - 1) selected++;
    }

    endwin();
    return 0;
}