#include "ansi_renderer.h"

#include <stdio.h>
#include <string.h>

static void write_str(Platform *platform, const char *value) {
    platform_write(platform, value, (int)strlen(value));
}

static void write_line_slice(Platform *platform, const TextLine *line, size_t left, int width) {
    if (width <= 0) return;
    if (left >= line->length) return;
    size_t available = line->length - left;
    size_t count = available < (size_t)width ? available : (size_t)width;
    platform_write(platform, line->data + left, (int)count);
}

void ansi_render_editor(Platform *platform, const Editor *editor) {
    char buf[512];
    int rows = editor->screen_rows;
    int cols = editor->screen_cols;
    if (rows < 3) rows = 3;
    if (cols < 10) cols = 10;

    write_str(platform, "\x1b[?25l");
    write_str(platform, "\x1b[H");

    int usable_rows = rows - 2;
    for (int screen_row = 0; screen_row < usable_rows; screen_row++) {
        size_t doc_row = editor->viewport.top_row + (size_t)screen_row;
        write_str(platform, "\x1b[K");

        if (doc_row < editor->document.line_count) {
            const TextLine *line = &editor->document.lines[doc_row];
            size_t left = doc_row == editor->cursor.row ? editor->viewport.active_line_left_col : 0;
            write_line_slice(platform, line, left, cols);
        } else {
            write_str(platform, "~");
        }

        write_str(platform, "\r\n");
    }

    write_str(platform, "\x1b[7m");
    snprintf(buf, sizeof(buf), " %.180s%s  Ln %zu, Col %zu ",
        editor->document.path == NULL ? "[No Name]" : editor->document.path,
        editor->document.dirty ? " *" : "",
        editor->cursor.row + 1,
        editor->cursor.col + 1);
    int len = (int)strlen(buf);
    if (len > cols) len = cols;
    platform_write(platform, buf, len);
    for (int i = len; i < cols; i++) write_str(platform, " ");
    write_str(platform, "\x1b[m");
    write_str(platform, "\r\n");

    write_str(platform, "\x1b[K");
    snprintf(buf, sizeof(buf), " %.250s   Ctrl+S Save | Ctrl+Q Quit", editor->status);
    len = (int)strlen(buf);
    if (len > cols) len = cols;
    platform_write(platform, buf, len);

    int cursor_screen_row = (int)(editor->cursor.row - editor->viewport.top_row) + 1;
    int cursor_screen_col = (int)(editor->cursor.col - editor->viewport.active_line_left_col) + 1;
    if (cursor_screen_col < 1) cursor_screen_col = 1;
    if (cursor_screen_col > cols) cursor_screen_col = cols;
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cursor_screen_row, cursor_screen_col);
    write_str(platform, buf);
    write_str(platform, "\x1b[?25h");
}
