#ifndef TEDIT_EDITOR_H
#define TEDIT_EDITOR_H

#include "document.h"

struct Platform;

typedef struct {
    size_t row;
    size_t col;
} Cursor;

typedef struct {
    size_t top_row;
    size_t active_line_left_col;
    size_t active_row;
} Viewport;

typedef enum {
    EDITOR_MODE_READ,
    EDITOR_MODE_WRITE
} EditorMode;

typedef struct {
    Document document;
    Cursor cursor;
    Viewport viewport;
    EditorMode mode;
    int screen_rows;
    int screen_cols;
    int should_quit;
    char status[256];
} Editor;

void editor_init(Editor *editor);
void editor_destroy(Editor *editor);
int editor_open(Editor *editor, const char *path);
void editor_enter_write_mode(Editor *editor);
void editor_handle_key(Editor *editor, int key);
void editor_update_viewport(Editor *editor);
int editor_run(Editor *editor, struct Platform *platform);

#endif
