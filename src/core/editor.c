#include "editor.h"
#include "platform/platform.h"
#include "renderer/ansi_renderer.h"

#include <stdio.h>
#include <string.h>

void editor_init(Editor *editor) {
    document_init(&editor->document);
    editor->cursor.row = 0;
    editor->cursor.col = 0;
    editor->viewport.top_row = 0;
    editor->viewport.active_line_left_col = 0;
    editor->viewport.active_row = 0;
    editor->mode = EDITOR_MODE_READ;
    editor->screen_rows = 24;
    editor->screen_cols = 80;
    editor->should_quit = 0;
    snprintf(editor->status, sizeof(editor->status), "Ready");
}

void editor_destroy(Editor *editor) {
    document_destroy(&editor->document);
}

int editor_open(Editor *editor, const char *path) {
    bool created_new = false;
    if (!document_load_path(&editor->document, path, &created_new)) {
        snprintf(editor->status, sizeof(editor->status), "Could not open %s", path);
        return 0;
    }
    editor->mode = created_new ? EDITOR_MODE_WRITE : EDITOR_MODE_READ;
    snprintf(editor->status, sizeof(editor->status), "%s %s", created_new ? "New file" : "Opened", path);
    return 1;
}

static void clamp_cursor(Editor *editor) {
    if (editor->cursor.row >= editor->document.line_count) {
        editor->cursor.row = editor->document.line_count == 0 ? 0 : editor->document.line_count - 1;
    }
    TextLine *line = &editor->document.lines[editor->cursor.row];
    if (editor->cursor.col > line->length) editor->cursor.col = line->length;
}

void editor_update_viewport(Editor *editor) {
    int usable_rows = editor->screen_rows - 2;
    if (usable_rows < 1) usable_rows = 1;

    if (editor->viewport.active_row != editor->cursor.row) {
        editor->viewport.active_row = editor->cursor.row;
        editor->viewport.active_line_left_col = 0;
    }

    if (editor->cursor.row < editor->viewport.top_row) {
        editor->viewport.top_row = editor->cursor.row;
    }

    if (editor->cursor.row >= editor->viewport.top_row + (size_t)usable_rows) {
        editor->viewport.top_row = editor->cursor.row - (size_t)usable_rows + 1;
    }

    size_t visible_cols = editor->screen_cols > 1 ? (size_t)editor->screen_cols : 1;
    if (editor->cursor.col < editor->viewport.active_line_left_col) {
        editor->viewport.active_line_left_col = editor->cursor.col;
    }
    if (editor->cursor.col >= editor->viewport.active_line_left_col + visible_cols) {
        editor->viewport.active_line_left_col = editor->cursor.col - visible_cols + 1;
    }
}

static void move_cursor(Editor *editor, int key) {
    switch (key) {
        case TEDIT_KEY_ARROW_LEFT:
            if (editor->cursor.col > 0) editor->cursor.col--;
            break;
        case TEDIT_KEY_ARROW_RIGHT:
            editor->cursor.col++;
            break;
        case TEDIT_KEY_ARROW_UP:
            if (editor->cursor.row > 0) editor->cursor.row--;
            break;
        case TEDIT_KEY_ARROW_DOWN:
            if (editor->cursor.row + 1 < editor->document.line_count) editor->cursor.row++;
            break;
        default:
            break;
    }
    clamp_cursor(editor);
}

void editor_enter_write_mode(Editor *editor) {
    editor->mode = EDITOR_MODE_WRITE;
    snprintf(editor->status, sizeof(editor->status), "WRITE mode");
}

static bool editor_is_editing_key(int key) {
    return key == TEDIT_KEY_BACKSPACE ||
           key == TEDIT_KEY_DELETE ||
           key == '\r' ||
           key == '\n' ||
           (key >= 32 && key <= 126);
}

void editor_handle_key(Editor *editor, int key) {
    if (key == TEDIT_KEY_CTRL_Q) {
        editor->should_quit = 1;
        return;
    }

    if (editor->mode == EDITOR_MODE_READ && (key == 'w' || key == 'W')) {
        editor_enter_write_mode(editor);
        return;
    }

    if (key == TEDIT_KEY_CTRL_S) {
        if (document_save(&editor->document)) snprintf(editor->status, sizeof(editor->status), "Saved");
        else snprintf(editor->status, sizeof(editor->status), "Save failed");
        return;
    }

    if (key >= TEDIT_KEY_ARROW_LEFT && key <= TEDIT_KEY_ARROW_DOWN) {
        move_cursor(editor, key);
        return;
    }

    if (editor->mode == EDITOR_MODE_READ && editor_is_editing_key(key)) {
        snprintf(editor->status, sizeof(editor->status), "READ mode - press w to edit");
        return;
    }

    if (key == TEDIT_KEY_BACKSPACE) {
        document_delete_char_before(&editor->document, editor->cursor.row, editor->cursor.col, &editor->cursor.row, &editor->cursor.col);
        return;
    }

    if (key == TEDIT_KEY_DELETE) {
        document_delete_char_at(&editor->document, editor->cursor.row, editor->cursor.col);
        return;
    }

    if (key == '\r' || key == '\n') {
        document_insert_newline(&editor->document, editor->cursor.row, editor->cursor.col, &editor->cursor.row, &editor->cursor.col);
        return;
    }

    if (key >= 32 && key <= 126) {
        document_insert_char(&editor->document, editor->cursor.row, editor->cursor.col, (char)key);
        editor->cursor.col++;
    }
}

int editor_run(Editor *editor, Platform *platform) {
    if (!platform_enter_raw_mode(platform)) {
        return 1;
    }

    while (!editor->should_quit) {
        platform_get_terminal_size(platform, &editor->screen_rows, &editor->screen_cols);
        clamp_cursor(editor);
        editor_update_viewport(editor);
        ansi_render_editor(platform, editor);

        int key = platform_read_key(platform);
        editor_handle_key(editor, key);
    }

    platform_leave_raw_mode(platform);
    return 0;
}
