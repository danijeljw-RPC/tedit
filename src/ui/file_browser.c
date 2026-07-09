#include "file_browser.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

static char *copy_string_fb(const char *value) {
    if (value == NULL) return NULL;
    size_t length = strlen(value);
    char *copy = malloc(length + 1);
    if (copy == NULL) return NULL;
    memcpy(copy, value, length + 1);
    return copy;
}

static bool join_path(const char *dir, const char *name, char *buffer, size_t capacity) {
    const char separator =
#ifdef _WIN32
        '\\';
#else
        '/';
#endif
    size_t dir_len = strlen(dir);
    bool has_separator = dir_len > 0 && (dir[dir_len - 1] == '/' || dir[dir_len - 1] == '\\');
    int written = snprintf(buffer, capacity, "%s%s%s", dir, has_separator ? "" : (char[]){separator, '\0'}, name);
    return written > 0 && (size_t)written < capacity;
}

static void file_browser_clear(FileBrowser *browser) {
    for (size_t i = 0; i < browser->entry_count; i++) {
        free(browser->entries[i].name);
        free(browser->entries[i].path);
    }
    browser->entry_count = 0;
    browser->selected = 0;
}

static bool file_browser_reserve(FileBrowser *browser, size_t capacity) {
    if (capacity <= browser->capacity) return true;
    FileBrowserEntry *next = realloc(browser->entries, capacity * sizeof(FileBrowserEntry));
    if (next == NULL) return false;
    browser->entries = next;
    browser->capacity = capacity;
    return true;
}

static bool file_browser_add(FileBrowser *browser, const char *name, const char *path, FileBrowserEntryKind kind) {
    if (browser->entry_count == browser->capacity) {
        size_t next_capacity = browser->capacity == 0 ? 16 : browser->capacity * 2;
        if (!file_browser_reserve(browser, next_capacity)) return false;
    }
    FileBrowserEntry *entry = &browser->entries[browser->entry_count++];
    entry->name = copy_string_fb(name);
    entry->path = copy_string_fb(path);
    entry->kind = kind;
    if (entry->name == NULL || entry->path == NULL) return false;
    return true;
}

static int compare_entries(const void *left_ptr, const void *right_ptr) {
    const FileBrowserEntry *left = (const FileBrowserEntry *)left_ptr;
    const FileBrowserEntry *right = (const FileBrowserEntry *)right_ptr;
    if (left->kind != right->kind) {
        if (left->kind == FILE_BROWSER_ENTRY_PARENT) return -1;
        if (right->kind == FILE_BROWSER_ENTRY_PARENT) return 1;
        if (left->kind == FILE_BROWSER_ENTRY_DIRECTORY) return -1;
        if (right->kind == FILE_BROWSER_ENTRY_DIRECTORY) return 1;
    }
    return strcmp(left->name, right->name);
}

void file_browser_init(FileBrowser *browser) {
    browser->current_dir = NULL;
    browser->entries = NULL;
    browser->entry_count = 0;
    browser->capacity = 0;
    browser->selected = 0;
    browser->open = false;
}

void file_browser_destroy(FileBrowser *browser) {
    file_browser_clear(browser);
    free(browser->entries);
    free(browser->current_dir);
    browser->entries = NULL;
    browser->current_dir = NULL;
    browser->capacity = 0;
    browser->open = false;
}

bool file_browser_open(FileBrowser *browser, const char *path) {
    file_browser_clear(browser);
    free(browser->current_dir);
    browser->current_dir = copy_string_fb(path == NULL || path[0] == '\0' ? "." : path);
    if (browser->current_dir == NULL) return false;

    if (!file_browser_add(browser, "..", "..", FILE_BROWSER_ENTRY_PARENT)) return false;

#ifdef _WIN32
    char pattern[4096];
    if (!join_path(browser->current_dir, "*", pattern, sizeof(pattern))) return false;
    WIN32_FIND_DATAA data;
    HANDLE handle = FindFirstFileA(pattern, &data);
    if (handle == INVALID_HANDLE_VALUE) return false;
    do {
        if (strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0) continue;
        char full_path[4096];
        if (!join_path(browser->current_dir, data.cFileName, full_path, sizeof(full_path))) {
            FindClose(handle);
            return false;
        }
        FileBrowserEntryKind kind = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
            ? FILE_BROWSER_ENTRY_DIRECTORY
            : FILE_BROWSER_ENTRY_FILE;
        if (!file_browser_add(browser, data.cFileName, full_path, kind)) {
            FindClose(handle);
            return false;
        }
    } while (FindNextFileA(handle, &data));
    FindClose(handle);
#else
    DIR *dir = opendir(browser->current_dir);
    if (dir == NULL) return false;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char full_path[4096];
        if (!join_path(browser->current_dir, entry->d_name, full_path, sizeof(full_path))) {
            closedir(dir);
            return false;
        }
        struct stat st;
        if (stat(full_path, &st) != 0) continue;
        FileBrowserEntryKind kind = S_ISDIR(st.st_mode) ? FILE_BROWSER_ENTRY_DIRECTORY : FILE_BROWSER_ENTRY_FILE;
        if (!file_browser_add(browser, entry->d_name, full_path, kind)) {
            closedir(dir);
            return false;
        }
    }
    closedir(dir);
#endif

    qsort(browser->entries, browser->entry_count, sizeof(FileBrowserEntry), compare_entries);
    browser->selected = 0;
    browser->open = true;
    return true;
}

void file_browser_close(FileBrowser *browser) {
    browser->open = false;
}

void file_browser_move_selection(FileBrowser *browser, int delta) {
    if (browser->entry_count == 0) return;
    if (delta > 0) {
        browser->selected = (browser->selected + 1) % browser->entry_count;
    } else if (delta < 0) {
        browser->selected = browser->selected == 0 ? browser->entry_count - 1 : browser->selected - 1;
    }
}

FileBrowserActivation file_browser_activate_selected(const FileBrowser *browser) {
    FileBrowserActivation activation;
    activation.kind = FILE_BROWSER_ACTIVATE_NONE;
    activation.path = NULL;
    if (browser->selected >= browser->entry_count) return activation;
    const FileBrowserEntry *entry = &browser->entries[browser->selected];
    activation.path = copy_string_fb(entry->path);
    if (activation.path == NULL) return activation;
    if (entry->kind == FILE_BROWSER_ENTRY_PARENT) activation.kind = FILE_BROWSER_ACTIVATE_PARENT;
    else if (entry->kind == FILE_BROWSER_ENTRY_DIRECTORY) activation.kind = FILE_BROWSER_ACTIVATE_DIRECTORY;
    else activation.kind = FILE_BROWSER_ACTIVATE_FILE;
    return activation;
}

void file_browser_activation_destroy(FileBrowserActivation *activation) {
    free(activation->path);
    activation->path = NULL;
    activation->kind = FILE_BROWSER_ACTIVATE_NONE;
}
