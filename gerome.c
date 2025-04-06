#include <ncurses.h>

#include "geom.h"
#include "ui.h"

int main(int argc, char** argv)
{
  struct ui_state state;
  ui_state_init(&state);
  
  bool has_color = init_curses();
  state.monochrome = !has_color;
  
  WINDOW* window;
  window = create_window(); 
  
  struct grm_disks disks;
  int err = grm_get_disks(&disks);
  if (err != 0) {
    printw("failed to get disks");
  }
  state.disk_count = disks.count;

  int input = 0;
  while (!state.exit) {
    werase(window);
    draw_window(window, &state, &disks);
    wrefresh(window);

    input = wgetch(window);
    update_state(window, &state, input);
  }

  grm_disks_free(&disks);

  delwin(window);
  endwin();

  return 0;  
}
