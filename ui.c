#include "ui.h"

#include <ncurses.h>
#include <sys/queue.h>
#include <libutil.h>

#include "geom.h"

#define DISK_PANE_MIN_WIDTH (23)
#define DISK_ENTRY_HEIGHT (5)
#define PARTITION_ENTRY_HEIGHT (10)

#define max(a, b) ({ __typeof__ (a) _a = a; __typeof__ (b) _b = b; _a > _b ? _a : _b; })

static void draw_box(WINDOW* window, int rows, int cols);
static void draw_disks_menu(WINDOW* window, struct ui_state* state, struct grm_disks* disks);
static void draw_disk_entry(WINDOW* window, struct grm_disk* disk, bool selected, bool monochrome);
static void draw_partitions(WINDOW *window, struct ui_state *state, struct grm_partitions *partitions);
static void draw_partition_entry(WINDOW* window, struct grm_partition* partition);
static int get_disks_pane_width(int window_cols);
static int visible_disk_count(int window_rows);
static int visible_partition_count(int window_rows);

int ui_state_init(struct ui_state *state)
{
  if (!state) {
    return -1;
  }

  state->exit = false;
  state->selected_disk = 0;
  state->scrolled_disks = 0;
  state->disk_count = 0;
  state->monochrome = true;

  return 0;
}

bool init_curses()
{
  initscr();

  bool has_color = has_colors();
  if (has_color) {
    start_color();

    init_pair(1, COLOR_BLACK, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    init_pair(3, COLOR_BLACK, COLOR_CYAN);

    bkgd(COLOR_PAIR(1));
    refresh();
  }

  cbreak();
  noecho();
  curs_set(0);

  return has_color;
}

WINDOW* create_window()
{
  WINDOW* window;
  window = newwin(LINES - 4, COLS - 10, 2, 5);
  keypad(window, TRUE);
  wbkgd(window, COLOR_PAIR(2));
  wrefresh(window);

  return window;
}

void draw_window(WINDOW* window, struct ui_state* state, struct grm_disks* disks)
{
  int rows;
  int cols;
  getmaxyx(window, rows, cols);

  box(window, 0, 0);
  
  mvwaddstr(window, 0, 3, "GEROME");

  mvwaddstr(window, rows - 1, 3, " Q - Quit ");
  mvwaddch(window, rows - 1, 16, ' ');
  waddch(window, ACS_UARROW);
  waddstr(window, " - Up ");
  mvwaddch(window, rows - 1, 27, ' ');
  waddch(window, ACS_DARROW);
  waddstr(window, " - Down ");
  
  draw_disks_menu(window, state, disks);

  int disks_seen = 0;
  struct grm_disk* disk;
  LIST_FOREACH(disk, &disks->grm_disk, grm_disk) {
    if (disks_seen == state->selected_disk) {
      draw_partitions(window, state, &disk->partitions);
      break;
    }
    disks_seen++;
  }
}

void update_state(WINDOW* window, struct ui_state* state, int input)
{
  switch (input) {
    case 'q':
      state->exit = true;
      break;
    case KEY_UP:
      if (state->selected_disk > 0) {
        state->selected_disk--;
        if (state->scrolled_disks > 0 && state->scrolled_disks == state->selected_disk + 1) {
          state->scrolled_disks--;
        }
      }
      break;
    case KEY_DOWN:
      if (state->selected_disk < state->disk_count - 1) {
        int visible_disks = visible_disk_count(getmaxy(window));

        state->selected_disk++;
        if (visible_disks <= state->disk_count - state->scrolled_disks  && visible_disks == state->selected_disk - state->scrolled_disks) {
          state->scrolled_disks++;
        }
      }
      break;
    case KEY_RESIZE:
      wresize(window, LINES - 4, COLS - 10);
      int visible_disks = visible_disk_count(getmaxy(window));
      if (visible_disks >= state->disk_count) {
        state->scrolled_disks = 0;
      } else if (visible_disks >= state->scrolled_disks) {
        state->scrolled_disks = state->selected_disk - visible_disks + 1;
      }
      
      erase();
      refresh();
      
      break;
  } 
}

static void draw_disks_menu(WINDOW* window, struct ui_state* state, struct grm_disks* disks)
{
  int rows;
  int cols;
  getmaxyx(window, rows, cols);

  int width = get_disks_pane_width(cols - 2);
  
  wmove(window, 1, 1);
  draw_box(window, rows - 2, width);

  mvwaddstr(window, 1, 4, "DISKS");

  wmove(window, 2, 2);

  if (!disks->count) {
    mvwaddstr(window, 3, 4, "No disks found");
    return;
  }

  int seen_disks = 0;
  int drawn_disks = 0;
  struct grm_disk* disk;
  LIST_FOREACH(disk, &disks->grm_disk, grm_disk) {
    if (seen_disks < state->scrolled_disks) {
      seen_disks++;
      continue;
    }
    
    bool is_selected = drawn_disks == (state->selected_disk - state->scrolled_disks);
    draw_disk_entry(window, disk, is_selected, state->monochrome);

    drawn_disks++;
    if (drawn_disks >= visible_disk_count(rows)) {
      break;
    }
    wmove(window, 2 + drawn_disks * DISK_ENTRY_HEIGHT, 2);
  }
}

static void draw_partitions(WINDOW *window, struct ui_state *state, struct grm_partitions *partitions)
{
  int rows;
  int cols;
  getmaxyx(window, rows, cols);

  int disks_width = get_disks_pane_width(cols - 2);
  int width = cols - disks_width - 2;
  
  wmove(window, 1, disks_width);
  draw_box(window, rows - 2, cols - disks_width -  1);  

  waddch(window, ACS_TTEE);
  wmove(window, rows - 2, disks_width);
  waddch(window, ACS_BTEE);

  mvwaddstr(window, 1, disks_width + 3, "PARTITIONS");

  if (!partitions->count) {
    mvwaddstr(window, 3, disks_width + 3, "No partitions found");
    return;
  }

  int visible_partitions = visible_partition_count(rows);
  int drawn_partitions = 0;
  struct grm_partition* partition;
  LIST_FOREACH(partition, &partitions->grm_partition, grm_partition) {
    if (drawn_partitions >= visible_partitions * 2) {
      break;
    }
    
    wmove(window, 3 + (drawn_partitions / 2) * PARTITION_ENTRY_HEIGHT, disks_width + 3 + (drawn_partitions & 1) * (width / 2) );
    draw_partition_entry(window, partition);
    drawn_partitions++;
  }

  if (partitions->count > visible_partitions * 2) {
    mvwaddstr(window, rows - 2, disks_width + 3, "Window too small to show all partitions");
  }
}

static void draw_box(WINDOW* window, int rows, int cols)
{
  int initial_row;
  int initial_col;
  getyx(window, initial_row, initial_col);
  
  wvline(window, 0, rows);
  whline(window, 0, cols);
  waddch(window, ACS_ULCORNER);

  wmove(window, initial_row + rows - 1, initial_col);
  whline(window, 0, cols);
  waddch(window, ACS_LLCORNER);

  wmove(window, initial_row, initial_col + cols - 1);
  wvline(window, 0, rows);
  waddch(window, ACS_URCORNER);
  
  wmove(window, initial_row + rows - 1, initial_col + cols - 1);
  waddch(window, ACS_LRCORNER);
  
  wmove(window, initial_row, initial_col);
}

static void draw_disk_entry(WINDOW* window, struct grm_disk* disk, bool selected, bool monochrome)
{ 
  int initial_row;
  int initial_col;
  getyx(window, initial_row, initial_col);

  int width = get_disks_pane_width(getmaxx(window) - 2) - 2;
  
  if (selected) {
    for (int i = 0; i < DISK_ENTRY_HEIGHT; i++) {
      mvwchgat(window, initial_row + i, initial_col, width, 0, 3, NULL);
    }
    wattron(window, COLOR_PAIR(3));
    wmove(window, initial_row, initial_col);
    draw_box(window, DISK_ENTRY_HEIGHT, width);
  }

  wattron(window, A_UNDERLINE);
  mvwaddstr(window, initial_row + 1, initial_col + 1, disk->name);
  wattroff(window, A_UNDERLINE);

  char buf[5];
  humanize_number(buf, sizeof(buf), disk->size, "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
  mvwprintw(window, initial_row + 2, initial_col + 1, "Size: %s", buf);
  
  humanize_number(buf, sizeof(buf), disk->sector_size, "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
  mvwprintw(window, initial_row + 3, initial_col + 1, "%s sector size", buf);
  
  wmove(window, initial_row, initial_col);
  wattroff(window, COLOR_PAIR(3));
}

static void draw_partition_entry(WINDOW* window, struct grm_partition* partition)
{
  int initial_row;
  int initial_col;
  getyx(window, initial_row, initial_col);  

  wattron(window, A_UNDERLINE);
  mvwaddstr(window, initial_row, initial_col, partition->name);
  wattroff(window, A_UNDERLINE);
  
  mvwprintw(window, initial_row + 1, initial_col, "Label: %s", partition->label);
  mvwprintw(window, initial_row + 2, initial_col, "Type: %s", partition->type);

  char buf[5];
  humanize_number(buf, sizeof(buf), partition->size, "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
  mvwprintw(window, initial_row + 3, initial_col, "Size: %s", buf);

  off_t sectors = partition->end - partition->start;
  mvwprintw(window, initial_row + 5, initial_col, "%ld sectors", sectors);  
  mvwprintw(window, initial_row + 6, initial_col, "Start: %ld", partition->start);  
  mvwprintw(window, initial_row + 7, initial_col, "End: %ld", partition->end);  

  wmove(window, initial_row, initial_col);
}

static int get_disks_pane_width(int window_cols)
{
  return max(DISK_PANE_MIN_WIDTH, window_cols / 3);
}

static int visible_disk_count(int window_rows)
{
  return (window_rows - 4) / DISK_ENTRY_HEIGHT;
}

static int visible_partition_count(int window_rows)
{
  return (window_rows - 4) / PARTITION_ENTRY_HEIGHT;
}
