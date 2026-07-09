#include "menu.h"
#include "platform/platform.h"

#include <string.h>

static const MenuItem file_items[] = {
    {"Open", MENU_COMMAND_OPEN, 0},
    {"Save", MENU_COMMAND_SAVE, 0},
    {"Save As", MENU_COMMAND_SAVE_AS, 0},
    {"Quit", MENU_COMMAND_QUIT, 0},
};

static const MenuItem edit_items[] = {
    {"Undo", MENU_COMMAND_UNDO, 0},
    {"Redo", MENU_COMMAND_REDO, 0},
    {"Copy", MENU_COMMAND_COPY, 0},
    {"Cut", MENU_COMMAND_CUT, 0},
    {"Paste", MENU_COMMAND_PASTE, 0},
};

static const MenuItem search_items[] = {
    {"Find", MENU_COMMAND_FIND, 0},
    {"Next", MENU_COMMAND_FIND_NEXT, 0},
    {"Previous", MENU_COMMAND_FIND_PREVIOUS, 0},
};

static const MenuItem view_items[] = {
    {"Line Numbers", MENU_COMMAND_TOGGLE_LINE_NUMBERS, 0},
    {"Whitespace", MENU_COMMAND_TOGGLE_WHITESPACE, 0},
};

static const MenuItem tools_items[] = {
    {"Literal Tab", MENU_COMMAND_TAB_LITERAL, 0},
    {"Tab 2 Spaces", MENU_COMMAND_TAB_TWO_SPACES, 0},
    {"Tab 4 Spaces", MENU_COMMAND_TAB_FOUR_SPACES, 0},
};

static const MenuItem help_items[] = {
    {"About", MENU_COMMAND_ABOUT, 0},
};

static const char *menu_labels[] = {
    "File", "Edit", "Search", "View", "Tools", "Help"
};

static const size_t menu_mnemonic_indexes[] = {
    0, 0, 0, 0, 0, 0
};

static const MenuItem *items_for_menu(MenuId id, size_t *count) {
    switch (id) {
        case MENU_FILE: *count = sizeof(file_items) / sizeof(file_items[0]); return file_items;
        case MENU_EDIT: *count = sizeof(edit_items) / sizeof(edit_items[0]); return edit_items;
        case MENU_SEARCH: *count = sizeof(search_items) / sizeof(search_items[0]); return search_items;
        case MENU_VIEW: *count = sizeof(view_items) / sizeof(view_items[0]); return view_items;
        case MENU_TOOLS: *count = sizeof(tools_items) / sizeof(tools_items[0]); return tools_items;
        case MENU_HELP: *count = sizeof(help_items) / sizeof(help_items[0]); return help_items;
        default: *count = 0; return NULL;
    }
}

void menu_bar_init(MenuBar *menu) {
    menu->open = false;
    menu->active_menu = MENU_FILE;
    menu->active_item = 0;
}

size_t menu_bar_menu_count(const MenuBar *menu) {
    (void)menu;
    return MENU_COUNT;
}

const char *menu_bar_menu_label(const MenuBar *menu, MenuId id) {
    (void)menu;
    if (id < 0 || id >= MENU_COUNT) return "";
    return menu_labels[id];
}

size_t menu_bar_menu_mnemonic_index(const MenuBar *menu, MenuId id) {
    (void)menu;
    if (id < 0 || id >= MENU_COUNT) return 0;
    return menu_mnemonic_indexes[id];
}

int menu_bar_menu_column(const MenuBar *menu, MenuId id) {
    (void)menu;
    if (id < 0 || id >= MENU_COUNT) return 1;
    int column = 1;
    for (int i = 0; i < (int)id; i++) {
        column += (int)strlen(menu_labels[i]) + 3;
    }
    return column;
}

size_t menu_bar_item_count(MenuId id) {
    size_t count = 0;
    items_for_menu(id, &count);
    return count;
}

const MenuItem *menu_bar_item(MenuId id, size_t index) {
    size_t count = 0;
    const MenuItem *items = items_for_menu(id, &count);
    if (items == NULL || index >= count) return NULL;
    return &items[index];
}

size_t menu_bar_item_mnemonic_index(MenuId id, size_t index) {
    const MenuItem *item = menu_bar_item(id, index);
    return item == NULL ? 0 : item->mnemonic_index;
}

static char lower_ascii(int key) {
    if (key >= 'A' && key <= 'Z') return (char)(key - 'A' + 'a');
    return (char)key;
}

MenuCommandId menu_bar_command_for_shortcut(MenuId id, int key) {
    size_t count = menu_bar_item_count(id);
    char wanted = lower_ascii(key);
    for (size_t i = 0; i < count; i++) {
        const MenuItem *item = menu_bar_item(id, i);
        if (item == NULL || item->label == NULL) continue;
        char actual = lower_ascii(item->label[item->mnemonic_index]);
        if (actual == wanted) return item->command;
    }
    return MENU_COMMAND_NONE;
}

bool menu_bar_open_shortcut(MenuBar *menu, int key) {
    switch (key) {
        case TEDIT_KEY_ALT_F: menu->active_menu = MENU_FILE; break;
        case TEDIT_KEY_ALT_E: menu->active_menu = MENU_EDIT; break;
        case TEDIT_KEY_ALT_S: menu->active_menu = MENU_SEARCH; break;
        case TEDIT_KEY_ALT_V: menu->active_menu = MENU_VIEW; break;
        case TEDIT_KEY_ALT_T: menu->active_menu = MENU_TOOLS; break;
        case TEDIT_KEY_ALT_H: menu->active_menu = MENU_HELP; break;
        default: return false;
    }
    menu->open = true;
    menu->active_item = 0;
    return true;
}

void menu_bar_close(MenuBar *menu) {
    menu->open = false;
    menu->active_item = 0;
}

void menu_bar_handle_key(MenuBar *menu, int key) {
    if (!menu->open) return;
    size_t count = menu_bar_item_count(menu->active_menu);
    if (count == 0) return;

    if (key == TEDIT_KEY_ARROW_DOWN) {
        menu->active_item = (menu->active_item + 1) % count;
    } else if (key == TEDIT_KEY_ARROW_UP) {
        menu->active_item = menu->active_item == 0 ? count - 1 : menu->active_item - 1;
    } else if (key == TEDIT_KEY_ARROW_RIGHT) {
        menu->active_menu = (MenuId)(((int)menu->active_menu + 1) % MENU_COUNT);
        menu->active_item = 0;
    } else if (key == TEDIT_KEY_ARROW_LEFT) {
        menu->active_menu = menu->active_menu == MENU_FILE ? MENU_HELP : (MenuId)(menu->active_menu - 1);
        menu->active_item = 0;
    }
}

MenuCommandId menu_bar_current_command(const MenuBar *menu) {
    if (!menu->open) return MENU_COMMAND_NONE;
    const MenuItem *item = menu_bar_item(menu->active_menu, menu->active_item);
    return item == NULL ? MENU_COMMAND_NONE : item->command;
}

MenuId menu_bar_active_menu(const MenuBar *menu) {
    return menu->active_menu;
}

size_t menu_bar_active_item(const MenuBar *menu) {
    return menu->active_item;
}
