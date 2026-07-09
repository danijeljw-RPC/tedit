#ifndef TEDIT_FILE_BROWSER_H
#define TEDIT_FILE_BROWSER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    FILE_BROWSER_ENTRY_PARENT,
    FILE_BROWSER_ENTRY_DIRECTORY,
    FILE_BROWSER_ENTRY_FILE
} FileBrowserEntryKind;

typedef struct {
    char *name;
    char *path;
    FileBrowserEntryKind kind;
} FileBrowserEntry;

typedef struct {
    char *current_dir;
    FileBrowserEntry *entries;
    size_t entry_count;
    size_t capacity;
    size_t selected;
    bool open;
} FileBrowser;

typedef enum {
    FILE_BROWSER_ACTIVATE_NONE,
    FILE_BROWSER_ACTIVATE_PARENT,
    FILE_BROWSER_ACTIVATE_DIRECTORY,
    FILE_BROWSER_ACTIVATE_FILE
} FileBrowserActivationKind;

typedef struct {
    FileBrowserActivationKind kind;
    char *path;
} FileBrowserActivation;

void file_browser_init(FileBrowser *browser);
void file_browser_destroy(FileBrowser *browser);
bool file_browser_open(FileBrowser *browser, const char *path);
void file_browser_close(FileBrowser *browser);
void file_browser_move_selection(FileBrowser *browser, int delta);
FileBrowserActivation file_browser_activate_selected(const FileBrowser *browser);
void file_browser_activation_destroy(FileBrowserActivation *activation);

#endif
