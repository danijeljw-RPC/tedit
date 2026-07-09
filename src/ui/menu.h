#ifndef TEDIT_MENU_H
#define TEDIT_MENU_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    MENU_FILE = 0,
    MENU_EDIT,
    MENU_SEARCH,
    MENU_VIEW,
    MENU_TOOLS,
    MENU_HELP,
    MENU_COUNT
} MenuId;

typedef enum {
    MENU_COMMAND_NONE = 0,
    MENU_COMMAND_OPEN,
    MENU_COMMAND_SAVE,
    MENU_COMMAND_SAVE_AS,
    MENU_COMMAND_QUIT,
    MENU_COMMAND_UNDO,
    MENU_COMMAND_REDO,
    MENU_COMMAND_COPY,
    MENU_COMMAND_CUT,
    MENU_COMMAND_PASTE,
    MENU_COMMAND_FIND,
    MENU_COMMAND_FIND_NEXT,
    MENU_COMMAND_FIND_PREVIOUS,
    MENU_COMMAND_TOGGLE_LINE_NUMBERS,
    MENU_COMMAND_TOGGLE_WHITESPACE,
    MENU_COMMAND_TAB_LITERAL,
    MENU_COMMAND_TAB_TWO_SPACES,
    MENU_COMMAND_TAB_FOUR_SPACES,
    MENU_COMMAND_ABOUT
} MenuCommandId;

typedef struct {
    const char *label;
    MenuCommandId command;
} MenuItem;

typedef struct {
    bool open;
    MenuId active_menu;
    size_t active_item;
} MenuBar;

void menu_bar_init(MenuBar *menu);
size_t menu_bar_menu_count(const MenuBar *menu);
const char *menu_bar_menu_label(const MenuBar *menu, MenuId id);
size_t menu_bar_item_count(MenuId id);
const MenuItem *menu_bar_item(MenuId id, size_t index);
bool menu_bar_open_shortcut(MenuBar *menu, int key);
void menu_bar_close(MenuBar *menu);
void menu_bar_handle_key(MenuBar *menu, int key);
MenuCommandId menu_bar_current_command(const MenuBar *menu);
MenuId menu_bar_active_menu(const MenuBar *menu);
size_t menu_bar_active_item(const MenuBar *menu);

#endif
