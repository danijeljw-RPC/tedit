#include "ansi_renderer.h"

#include <stdio.h>
#include <string.h>

static void write_str(Platform *platform, const char *value) {
    platform_write(platform, value, (int)strlen(value));
}

static void write_highlight_style(Platform *platform, EditorHighlight highlight) {
    if (highlight == TEDIT_HIGHLIGHT_SELECTION) {
        write_str(platform, "\x1b[7m");
    } else if (highlight == TEDIT_HIGHLIGHT_SEARCH) {
        write_str(platform, "\x1b[43;30m");
    } else {
        write_str(platform, "\x1b[m");
    }
}

static void write_line_slice(Platform *platform, const Editor *editor, size_t row, const TextLine *line, size_t left, int width) {
    if (width <= 0) return;
    if (left >= line->length) return;
    size_t available = line->length - left;
    size_t count = available < (size_t)width ? available : (size_t)width;
    EditorHighlight current = TEDIT_HIGHLIGHT_NONE;
    for (size_t i = 0; i < count; i++) {
        size_t col = left + i;
        EditorHighlight next = editor_highlight_at(editor, row, col);
        if (next != current) {
            write_highlight_style(platform, next);
            current = next;
        }
        platform_write(platform, line->data + col, 1);
    }
    if (current != TEDIT_HIGHLIGHT_NONE) write_highlight_style(platform, TEDIT_HIGHLIGHT_NONE);
}

static const char *mode_label(const Editor *editor) {
    return editor->mode == EDITOR_MODE_WRITE ? "WRITE" : "READ";
}

static const char *line_ending_label(const Document *document) {
    return document->line_ending == DOCUMENT_LINE_ENDING_CRLF ? "CRLF" : "LF";
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
            write_line_slice(platform, editor, doc_row, line, left, cols);
        } else {
            write_str(platform, "~");
        }

        write_str(platform, "\r\n");
    }

    write_str(platform, "\x1b[7m");
    snprintf(buf, sizeof(buf), " %.160s%s  Ln %zu, Col %zu  UTF-8 %s %s ",
        editor->document.path == NULL ? "[No Name]" : editor->document.path,
        editor->document.dirty ? " *" : "",
        editor->cursor.row + 1,
        editor->cursor.col + 1,
        line_ending_label(&editor->document),
        mode_label(editor));
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
