#ifndef GEROME_UI_H
#define GEROME_UI_H

#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>

#include "geom.h"

struct ui_state {
  bool exit;
  
  int16_t selected_disk;
  int16_t scrolled_disks;

  int disk_count;

  bool monochrome;
};

int ui_state_init(struct ui_state* state);

bool init_curses();

WINDOW* create_window();
void draw_window(WINDOW* window, struct ui_state* state, struct grm_disks* disks);
void update_state(WINDOW* window, struct ui_state* state, int input);

#endif
