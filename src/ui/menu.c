#include "menu.h"
#include "platform/platform.h"

static const MenuItem file_items[] = {
    {"Open", MENU_COMMAND_OPEN},
    {"Save", MENU_COMMAND_SAVE},
    {"Save As", MENU_COMMAND_SAVE_AS},
    {"Quit", MENU_COMMAND_QUIT},
};

static const MenuItem edit_items[] = {
    {"Undo", MENU_COMMAND_UNDO},
    {"Redo", MENU_COMMAND_REDO},
    {"Copy", MENU_COMMAND_COPY},
    {"Cut", MENU_COMMAND_CUT},
    {"Paste", MENU_COMMAND_PASTE},
};

static const MenuItem search_items[] = {
    {"Find", MENU_COMMAND_FIND},
    {"Next", MENU_COMMAND_FIND_NEXT},
    {"Previous", MENU_COMMAND_FIND_PREVIOUS},
};

static const MenuItem view_items[] = {
    {"Line Numbers", MENU_COMMAND_TOGGLE_LINE_NUMBERS},
    {"Whitespace", MENU_COMMAND_TOGGLE_WHITESPACE},
};

static const MenuItem tools_items[] = {
    {"Literal Tab", MENU_COMMAND_TAB_LITERAL},
    {"Tab 2 Spaces", MENU_COMMAND_TAB_TWO_SPACES},
    {"Tab 4 Spaces", MENU_COMMAND_TAB_FOUR_SPACES},
};

static const MenuItem help_items[] = {
    {"About", MENU_COMMAND_ABOUT},
};

static const char *menu_labels[] = {
    "File", "Edit", "Search", "View", "Tools", "Help"
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
