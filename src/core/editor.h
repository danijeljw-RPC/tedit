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
} Viewport;

typedef struct {
    Document document;
    Cursor cursor;
    Viewport viewport;
    int screen_rows;
    int screen_cols;
    int should_quit;
    char status[256];
} Editor;

void editor_init(Editor *editor);
void editor_destroy(Editor *editor);
int editor_open(Editor *editor, const char *path);
int editor_run(Editor *editor, struct Platform *platform);

#endif
