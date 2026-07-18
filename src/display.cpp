#include "config.h"
#include "kill_utils.h"
#include "process.h"
#include <cctype>
#include <chrono>
#include <ncurses.h>
#include <unistd.h>

void display_processes(const int header_rows, const int footer_rows, const int selected,
                       const int limit, const ProcessGroupVec &groups) {
  // calculate available rows
  const int HEADER_ROWS = header_rows;
  const int FOOTER_ROWS = footer_rows;
  int available_rows = LINES - HEADER_ROWS - FOOTER_ROWS;

  // calculate scroll offset
  // Only scroll when selected moves outside the visible window
  static int scroll_offset = 0;

  int app_index = 0;

  if (selected < scroll_offset) {
    scroll_offset = selected;
  } else if (selected >= scroll_offset + available_rows) {
    scroll_offset = selected - available_rows + 1;
  }

  for (const auto &app_values : groups) {

    // This would skip indices of the groups until reaches scroll_offset

    if (app_index < scroll_offset) {
      app_index++;
      continue;
    }
    // stop if we've filled the screen
    // EX: 7th app would be the last to be shown if 6 available rows and scroll_offset = 1

    if (app_index - scroll_offset >= available_rows) {
      break;
    }

    const std::string &app_name = app_values.name;
    const long &total_mb = app_values.total_mb;
    const int &num_processes = app_values.num_processes;

    std::string line_text = app_name + " " + std::to_string(total_mb) + " MB (" +
                            std::to_string(num_processes) + " processes)";

    if (total_mb > limit) {
      line_text += " Warning, process group over configured limit";
    }

    // EX: app_index is 1 (second app) and scroll_offset = 1
    // 1 - 1 + 2 = 2
    int display_row = app_index - scroll_offset + HEADER_ROWS;

    if (app_index == selected) {
      attron(A_REVERSE);
      mvprintw(display_row, 0, "-> %s", line_text.c_str());
      attroff(A_REVERSE);
      mvprintw(LINES - 3, 0, "Currently Selected Process: %s", app_name.c_str());
    } else {
      mvprintw(display_row, 0, line_text.c_str());
    }
    app_index++;
  }
}

void search_display(const int limit) {
  clear();
  const int header_rows = 5;
  const int footer_rows = 4;
  Config config = load_config("warden.conf");
  // Mode can either be select mode where user is searching for a process
  // or kill mode where a user can kill a process after pressing enter on one.
  // Initialize with select mode.
  std::string mode = "search";
  std::string line(COLS, '-');
  set_escdelay(25);

  std::string search_query = "";
  int selected = 0;

  ProcessGroupVec local_groups = group_processes(scan_processes(config.protected_processes));
  auto last_refresh = std::chrono::steady_clock::now();

  while (true) {
    // Rescan every 5 seconds, same as main()
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_refresh).count();
    if (elapsed >= 5) {
      local_groups = group_processes(scan_processes(config.protected_processes));
      last_refresh = now;
    }

    clear();

    ProcessGroupVec filtered_groups = search_processes(search_query, local_groups);
    int filtered_size = static_cast<int>(filtered_groups.size());

    if (selected >= filtered_size) {
      if (filtered_size == 0) {
        selected = 0;
      } else {
        selected = filtered_size - 1;
      }
    }

    display_processes(header_rows, footer_rows, selected, limit, filtered_groups);

    mvprintw(2, 0, "%s", line.c_str());
    mvprintw(4, 0, "%s", line.c_str());
    mvprintw(LINES - footer_rows, 0, "%s", line.c_str());
    mvprintw(LINES - 2, 0, "%s", line.c_str());

    // Display the current search string, live, every loop
    mvprintw(3, 0, "Search: %s", search_query.c_str());
    // clrtoeol(); // clear any leftover chars from a previous longer string

    // Navigation bar will have to go through a process to allow a user to kill a process.
    // First, user presses enter to confirm a certain process.
    // Then, a new navigation will allow user to kill the process or go back to selecting.
    if (mode == "search") {
      mvprintw(0, 0, "Search a Process!");
      mvprintw(1, 0, "Begin typing the name of the process you want to view");
      mvprintw(LINES - 1, 0,
               "[up/down] navigate  [enter] choose selected process  [esc] exit search");

    } else {
      mvprintw(0, 0, "You have selected a process");
      mvprintw(1, 0, "Choose an operation");
      mvprintw(LINES - 1, 0, "[b] back to selecting  [k] kill");
    }
    refresh();

    int key = getch();

    if (mode == "search") {
      if (key == KEY_UP) {
        if (selected > 0) {
          selected--;
        }
      } else if (key == KEY_DOWN) {
        if (selected < (int)filtered_groups.size() - 1) {
          selected++;
        }
      } else if (key == 27) {
        // Go back to home screen if user presses escape key
        return;
      } else if (key == '\n') {
        // Mode should be kill mode if user presses enter key
        if (filtered_size > 0) {
          mode = "kill";
        }
      } else if (key == KEY_BACKSPACE || key == 127 || key == 8) {
        if (!search_query.empty()) {
          search_query.pop_back();
        }
      } else if (std::isprint(key)) {
        search_query += static_cast<char>(key);
      }
    }

    if (mode == "kill") {
      // Confirmation prompt for kill action
      if (key == 'k') {
        // Show confirmation prompt
        mvprintw(LINES - 3, 0, "Are you sure you want to kill this group? (y/n) ");
        refresh();
        // Set getch to blocking mode
        timeout(-1);
        int confirm = getch();
        // Restore timeout to 100ms
        timeout(100);

        mvprintw(LINES - 3, 0, "%*s", COLS, ""); // Overwrite old text with spaces
        if (confirm == 'y' || confirm == 'Y') {
          kill_process_group(local_groups[selected].name,
                             scan_processes(config.protected_processes));
          mvprintw(LINES - 3, 0, "Process group killed!");
          // Rescan immediately after a kill so the list reflects it right away
          local_groups = group_processes(scan_processes(config.protected_processes));
          last_refresh = std::chrono::steady_clock::now();
        } else {
          mvprintw(LINES - 3, 0, "Kill cancelled.");
        }
        refresh();

        // Set mode back to search once deleted
        mode = "search";
        // Wait briefly so user can see the result
        napms(1000);
      }

      if (key == 'b') {
        mode = "search";
      }
    }
  }
}

int main() {
  initscr();
  noecho();             // doesnt print keypresses
  cbreak();             // makes keypresses instant without needing enter
  keypad(stdscr, true); // allows for arrow keys
  timeout(100); // Controls how long getch() waits for a keypress before giving up and returning -1.
  Config config = load_config("warden.conf");
  long limit = config.limit;

  int selected = 0; // currently selected item

  // Initial scan before loop
  auto last_refresh = std::chrono::steady_clock::now();
  auto groups = group_processes(scan_processes(config.protected_processes));

  while (true) {
    // Only scan the processes every 5 seconds instead of every press of a key
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_refresh).count();

    if (elapsed >= 5) {
      groups = group_processes(scan_processes(config.protected_processes));
      last_refresh = now;
    }

    clear(); // wipe the canvas

    std::string line(COLS, '-');
    mvprintw(0, 0, "Number of Running Procceses: %d", static_cast<int>(groups.size()));
    mvprintw(1, 0, "Warden - Select a Process:");
    mvprintw(2, 0, "%s", line.c_str());

    int header_rows = 3;
    int footer_rows = 4;

    display_processes(header_rows, footer_rows, selected, limit, groups);

    mvprintw(LINES - footer_rows, 0, "%s", line.c_str());

    mvprintw(LINES - 2, 0, "%s", line.c_str());
    mvprintw(LINES - 1, 0, "[up/down] navigate  [s]search  [k] kill  [q] quit");

    refresh(); // Prints to canvas

    int key = getch();

    if (key == 's') {
      search_display(limit);
    }

    if (key == 'q') {
      break;
    }
    if (key == KEY_UP && selected > 0) {
      selected--;
    }
    if (key == KEY_DOWN && selected < (int)groups.size() - 1) {
      selected++;
    }

    // Confirmation prompt for kill action
    if (key == 'k') {
      // Show confirmation prompt
      mvprintw(LINES - 3, 0, "Are you sure you want to kill this group? (y/n) ");
      refresh();
      // Set getch to blocking mode
      timeout(-1);
      int confirm = getch();
      // Restore timeout to 100ms
      timeout(100);

      mvprintw(LINES - 3, 0, "%*s", COLS, ""); // Overwrite old text with spaces
      if (confirm == 'y' || confirm == 'Y') {
        kill_process_group(groups[selected].name, scan_processes(config.protected_processes));
        mvprintw(LINES - 3, 0, "Process group killed!");
        groups = group_processes(scan_processes(config.protected_processes));
        last_refresh = std::chrono::steady_clock::now();
      } else {
        mvprintw(LINES - 3, 0, "Kill cancelled.");
      }
      refresh();
      // Wait briefly so user can see the result
      napms(1000);
    }
  }

  endwin();
  return 0;
}
