#include "editor.h"
#include "platform/platform.h"
#include "renderer/ansi_renderer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void editor_init(Editor *editor) {
    document_init(&editor->document);
    editor->cursor.row = 0;
    editor->cursor.col = 0;
    editor->viewport.top_row = 0;
    editor->viewport.active_line_left_col = 0;
    editor->viewport.active_row = 0;
    undo_stack_init(&editor->undo);
    search_state_init(&editor->search);
    editor->selection.active = false;
    editor->selection.anchor.row = 0;
    editor->selection.anchor.col = 0;
    editor->selection.cursor = editor->selection.anchor;
    editor->clipboard = NULL;
    editor->clipboard_length = 0;
    editor->prompt_mode = EDITOR_PROMPT_NONE;
    editor->prompt_buffer[0] = '\0';
    editor->replacement_buffer[0] = '\0';
    editor->mode = EDITOR_MODE_READ;
    editor->screen_rows = 24;
    editor->screen_cols = 80;
    editor->should_quit = 0;
    snprintf(editor->status, sizeof(editor->status), "Ready");
}

void editor_destroy(Editor *editor) {
    search_state_destroy(&editor->search);
    undo_stack_destroy(&editor->undo);
    free(editor->clipboard);
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

static DocumentPosition cursor_to_position(Cursor cursor);
int editor_has_selection(const Editor *editor);

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
    size_t usable_rows = editor->screen_rows > 2 ? (size_t)(editor->screen_rows - 2) : 1;

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
        case TEDIT_KEY_HOME:
            editor->cursor.col = 0;
            break;
        case TEDIT_KEY_END:
            editor->cursor.col = editor->document.lines[editor->cursor.row].length;
            break;
        case TEDIT_KEY_PAGE_UP:
            editor->cursor.row = editor->cursor.row > usable_rows ? editor->cursor.row - usable_rows : 0;
            break;
        case TEDIT_KEY_PAGE_DOWN:
            editor->cursor.row += usable_rows;
            if (editor->cursor.row >= editor->document.line_count) {
                editor->cursor.row = editor->document.line_count == 0 ? 0 : editor->document.line_count - 1;
            }
            break;
        case TEDIT_KEY_CTRL_HOME:
            editor->cursor.row = 0;
            editor->cursor.col = 0;
            break;
        case TEDIT_KEY_CTRL_END:
            editor->cursor.row = editor->document.line_count == 0 ? 0 : editor->document.line_count - 1;
            editor->cursor.col = editor->document.lines[editor->cursor.row].length;
            break;
        default:
            break;
    }
    clamp_cursor(editor);
}

static int shift_arrow_to_plain_arrow(int key) {
    switch (key) {
        case TEDIT_KEY_SHIFT_ARROW_LEFT: return TEDIT_KEY_ARROW_LEFT;
        case TEDIT_KEY_SHIFT_ARROW_RIGHT: return TEDIT_KEY_ARROW_RIGHT;
        case TEDIT_KEY_SHIFT_ARROW_UP: return TEDIT_KEY_ARROW_UP;
        case TEDIT_KEY_SHIFT_ARROW_DOWN: return TEDIT_KEY_ARROW_DOWN;
        default: return 0;
    }
}

static void editor_extend_selection(Editor *editor, int key) {
    DocumentPosition anchor = editor_has_selection(editor) ? editor->selection.anchor : cursor_to_position(editor->cursor);
    int plain_key = shift_arrow_to_plain_arrow(key);
    if (plain_key == 0) return;
    move_cursor(editor, plain_key);
    editor_set_selection(editor, anchor, cursor_to_position(editor->cursor));
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
           key == '\t' ||
           (key >= 32 && key <= 126);
}

static void editor_start_prompt(Editor *editor, EditorPromptMode mode, const char *status) {
    editor->prompt_mode = mode;
    editor->prompt_buffer[0] = '\0';
    snprintf(editor->status, sizeof(editor->status), "%s", status);
}

static DocumentPosition cursor_to_position(Cursor cursor) {
    DocumentPosition position = {cursor.row, cursor.col};
    return position;
}

static Cursor position_to_cursor(DocumentPosition position) {
    Cursor cursor = {position.row, position.col};
    return cursor;
}

void editor_clear_selection(Editor *editor) {
    editor->selection.active = false;
    editor->selection.anchor = cursor_to_position(editor->cursor);
    editor->selection.cursor = editor->selection.anchor;
}

void editor_set_selection(Editor *editor, DocumentPosition anchor, DocumentPosition cursor) {
    editor->selection.anchor = document_clamp_position(&editor->document, anchor);
    editor->selection.cursor = document_clamp_position(&editor->document, cursor);
    editor->selection.active = document_position_compare(editor->selection.anchor, editor->selection.cursor) != 0;
}

int editor_has_selection(const Editor *editor) {
    return editor->selection.active &&
           document_position_compare(editor->selection.anchor, editor->selection.cursor) != 0;
}

const char *editor_clipboard_text(const Editor *editor) {
    return editor->clipboard == NULL ? "" : editor->clipboard;
}

static DocumentRange editor_selection_range(const Editor *editor) {
    DocumentRange range = {editor->selection.anchor, editor->selection.cursor};
    return document_range_normalize(&editor->document, range);
}

static bool editor_set_clipboard(Editor *editor, const char *text, size_t length) {
    char *copy = malloc(length + 1);
    if (copy == NULL) return false;
    if (length > 0) memcpy(copy, text, length);
    copy[length] = '\0';
    free(editor->clipboard);
    editor->clipboard = copy;
    editor->clipboard_length = length;
    return true;
}

static bool range_contains_position(DocumentRange range, size_t row, size_t col) {
    if (row < range.start.row || row > range.end.row) return false;
    if (range.start.row == range.end.row) {
        return row == range.start.row && col >= range.start.col && col < range.end.col;
    }
    if (row == range.start.row) return col >= range.start.col;
    if (row == range.end.row) return col < range.end.col;
    return true;
}

static bool search_match_covers_col(const TextLine *line, const char *query, size_t query_length, size_t col) {
    if (query == NULL || query_length == 0 || col >= line->length) return false;
    for (size_t start = 0; start + query_length <= line->length; start++) {
        if (col >= start && col < start + query_length && memcmp(line->data + start, query, query_length) == 0) {
            return true;
        }
    }
    return false;
}

EditorHighlight editor_highlight_at(const Editor *editor, size_t row, size_t col) {
    if (row >= editor->document.line_count) return TEDIT_HIGHLIGHT_NONE;
    if (editor_has_selection(editor)) {
        DocumentRange range = editor_selection_range(editor);
        if (range_contains_position(range, row, col)) return TEDIT_HIGHLIGHT_SELECTION;
    }
    if (search_match_covers_col(&editor->document.lines[row], editor->search.query, editor->search.query_length, col)) {
        return TEDIT_HIGHLIGHT_SEARCH;
    }
    return TEDIT_HIGHLIGHT_NONE;
}

static DocumentPosition position_after_bytes(DocumentPosition start, const char *data, size_t length) {
    DocumentPosition position = start;
    for (size_t i = 0; i < length; i++) {
        if (data[i] == '\n') {
            position.row++;
            position.col = 0;
        } else {
            position.col++;
        }
    }
    return position;
}

static bool editor_apply_replace(Editor *editor, DocumentRange range, const char *text, size_t length, bool merge_insert) {
    range = document_range_normalize(&editor->document, range);
    size_t deleted_length = 0;
    char *deleted = document_extract_range_with_length(&editor->document, range, &deleted_length);
    if (deleted == NULL) return false;

    UndoRecord record;
    record.start = range.start;
    record.end_before = range.end;
    record.cursor_before = cursor_to_position(editor->cursor);
    record.deleted = deleted;
    record.deleted_length = deleted_length;
    record.inserted = malloc(length + 1);
    if (record.inserted == NULL) {
        free(deleted);
        return false;
    }
    if (length > 0) memcpy(record.inserted, text, length);
    record.inserted[length] = '\0';
    record.inserted_length = length;

    DocumentPosition after = document_replace_range(&editor->document, range, text, length);
    editor->search.has_current = false;
    record.cursor_after = after;
    editor->cursor = position_to_cursor(after);
    clamp_cursor(editor);

    if (!undo_stack_push_edit(&editor->undo, record, merge_insert)) {
        return false;
    }
    return true;
}

static bool editor_insert_text(Editor *editor, const char *text, size_t length, bool merge_insert) {
    if (editor_has_selection(editor)) {
        DocumentRange range = editor_selection_range(editor);
        bool ok = editor_apply_replace(editor, range, text, length, false);
        editor_clear_selection(editor);
        return ok;
    }
    DocumentPosition position = cursor_to_position(editor->cursor);
    DocumentRange range = {position, position};
    return editor_apply_replace(editor, range, text, length, merge_insert);
}

static DocumentPosition document_end_position(const Document *doc) {
    DocumentPosition end = {0, 0};
    if (doc->line_count == 0) return end;
    end.row = doc->line_count - 1;
    end.col = doc->lines[end.row].length;
    return end;
}

static char *replace_all_bytes(const char *source, size_t source_length, const char *query, const char *replacement, int *replace_count, size_t *out_length) {
    size_t query_length = strlen(query);
    size_t replacement_length = strlen(replacement);
    *replace_count = 0;
    if (query_length == 0) return NULL;

    for (size_t i = 0; i + query_length <= source_length;) {
        if (memcmp(source + i, query, query_length) == 0) {
            (*replace_count)++;
            i += query_length;
        } else {
            i++;
        }
    }
    if (*replace_count == 0) return NULL;

    size_t output_length = source_length + ((size_t)*replace_count * replacement_length) - ((size_t)*replace_count * query_length);
    char *output = malloc(output_length + 1);
    if (output == NULL) return NULL;

    size_t out = 0;
    for (size_t i = 0; i < source_length;) {
        if (i + query_length <= source_length && memcmp(source + i, query, query_length) == 0) {
            memcpy(output + out, replacement, replacement_length);
            out += replacement_length;
            i += query_length;
        } else {
            output[out++] = source[i++];
        }
    }
    output[out] = '\0';
    if (out_length != NULL) *out_length = output_length;
    return output;
}

static bool editor_delete_backward(Editor *editor) {
    if (editor_has_selection(editor)) {
        DocumentRange range = editor_selection_range(editor);
        bool ok = editor_apply_replace(editor, range, "", 0, false);
        editor_clear_selection(editor);
        return ok;
    }
    DocumentPosition end = cursor_to_position(editor->cursor);
    DocumentPosition start = end;
    if (end.col > 0) {
        start.col--;
    } else if (end.row > 0) {
        start.row--;
        start.col = editor->document.lines[start.row].length;
    } else {
        return false;
    }
    DocumentRange range = {start, end};
    return editor_apply_replace(editor, range, "", 0, false);
}

static bool editor_delete_forward(Editor *editor) {
    if (editor_has_selection(editor)) {
        DocumentRange range = editor_selection_range(editor);
        bool ok = editor_apply_replace(editor, range, "", 0, false);
        editor_clear_selection(editor);
        return ok;
    }
    DocumentPosition start = cursor_to_position(editor->cursor);
    DocumentPosition end = start;
    if (start.row >= editor->document.line_count) return false;
    TextLine *line = &editor->document.lines[start.row];
    if (start.col < line->length) {
        end.col++;
    } else if (start.row + 1 < editor->document.line_count) {
        end.row++;
        end.col = 0;
    } else {
        return false;
    }
    DocumentRange range = {start, end};
    return editor_apply_replace(editor, range, "", 0, false);
}

static bool editor_copy_selection(Editor *editor) {
    if (!editor_has_selection(editor)) return false;
    DocumentRange range = editor_selection_range(editor);
    size_t text_length = 0;
    char *text = document_extract_range_with_length(&editor->document, range, &text_length);
    if (text == NULL) return false;
    bool ok = editor_set_clipboard(editor, text, text_length);
    free(text);
    if (ok) snprintf(editor->status, sizeof(editor->status), "Copied");
    return ok;
}

static bool editor_cut_selection(Editor *editor) {
    if (!editor_copy_selection(editor)) return false;
    if (editor->mode == EDITOR_MODE_READ) {
        snprintf(editor->status, sizeof(editor->status), "READ mode - press w to edit");
        return false;
    }
    DocumentRange range = editor_selection_range(editor);
    bool ok = editor_apply_replace(editor, range, "", 0, false);
    editor_clear_selection(editor);
    if (ok) snprintf(editor->status, sizeof(editor->status), "Cut");
    return ok;
}

static bool editor_paste_clipboard(Editor *editor) {
    if (editor->clipboard == NULL || editor->clipboard_length == 0) return false;
    return editor_insert_text(editor, editor->clipboard, editor->clipboard_length, false);
}

static bool editor_cut_line(Editor *editor) {
    if (editor_has_selection(editor)) return editor_cut_selection(editor);
    if (editor->mode == EDITOR_MODE_READ) {
        snprintf(editor->status, sizeof(editor->status), "READ mode - press w to edit");
        return false;
    }
    DocumentPosition start = {editor->cursor.row, 0};
    DocumentPosition end = {editor->cursor.row, editor->document.lines[editor->cursor.row].length};
    DocumentRange range = {start, end};
    size_t text_length = 0;
    char *text = document_extract_range_with_length(&editor->document, range, &text_length);
    if (text == NULL) return false;
    bool copied = editor_set_clipboard(editor, text, text_length);
    free(text);
    if (!copied) return false;
    bool ok = editor_apply_replace(editor, range, "", 0, false);
    editor_clear_selection(editor);
    if (ok) snprintf(editor->status, sizeof(editor->status), "Line cut");
    return ok;
}

static void editor_undo(Editor *editor) {
    UndoRecord record;
    if (!undo_stack_pop_undo(&editor->undo, &record)) {
        snprintf(editor->status, sizeof(editor->status), "Nothing to undo");
        return;
    }

    DocumentPosition inserted_end = position_after_bytes(record.start, record.inserted, record.inserted_length);
    DocumentRange inverse_range = {record.start, inserted_end};
    document_replace_range(&editor->document, inverse_range, record.deleted, record.deleted_length);
    editor->search.has_current = false;
    editor->cursor = position_to_cursor(record.cursor_before);
    clamp_cursor(editor);
    undo_stack_push_redo(&editor->undo, record);
    snprintf(editor->status, sizeof(editor->status), "Undone");
}

static void editor_redo(Editor *editor) {
    UndoRecord record;
    if (!undo_stack_pop_redo(&editor->undo, &record)) {
        snprintf(editor->status, sizeof(editor->status), "Nothing to redo");
        return;
    }

    DocumentRange redo_range = {record.start, record.end_before};
    document_replace_range(&editor->document, redo_range, record.inserted, record.inserted_length);
    editor->search.has_current = false;
    editor->cursor = position_to_cursor(record.cursor_after);
    clamp_cursor(editor);
    undo_stack_push_undo_record(&editor->undo, record);
    snprintf(editor->status, sizeof(editor->status), "Redone");
}

int editor_search_set_query(Editor *editor, const char *query) {
    int result = search_set_query(&editor->search, query) ? 1 : 0;
    editor->search.match_count = search_count_matches(&editor->document, &editor->search);
    if (result) snprintf(editor->status, sizeof(editor->status), "Find: %s", query);
    else snprintf(editor->status, sizeof(editor->status), "Find query empty");
    return result;
}

int editor_find_next(Editor *editor) {
    if (!search_find_next(&editor->document, &editor->search, cursor_to_position(editor->cursor))) {
        snprintf(editor->status, sizeof(editor->status), "No match");
        return 0;
    }
    editor->cursor = position_to_cursor(editor->search.current.start);
    snprintf(editor->status, sizeof(editor->status), "Match %zu found", editor->search.match_count);
    return 1;
}

int editor_find_previous(Editor *editor) {
    if (!search_find_previous(&editor->document, &editor->search, cursor_to_position(editor->cursor))) {
        snprintf(editor->status, sizeof(editor->status), "No match");
        return 0;
    }
    editor->cursor = position_to_cursor(editor->search.current.start);
    snprintf(editor->status, sizeof(editor->status), "Match %zu found", editor->search.match_count);
    return 1;
}

int editor_replace_current(Editor *editor, const char *replacement) {
    if (editor->mode == EDITOR_MODE_READ) {
        snprintf(editor->status, sizeof(editor->status), "READ mode - press w to edit");
        return 0;
    }
    if (!editor->search.has_current) {
        if (!editor_find_next(editor)) return 0;
    }
    size_t replacement_length = strlen(replacement);
    if (!editor_apply_replace(editor, editor->search.current, replacement, replacement_length, false)) return 0;
    editor->search.has_current = false;
    editor->search.match_count = search_count_matches(&editor->document, &editor->search);
    snprintf(editor->status, sizeof(editor->status), "Replaced");
    return 1;
}

int editor_replace_all_confirmed(Editor *editor, const char *replacement) {
    if (editor->mode == EDITOR_MODE_READ) {
        snprintf(editor->status, sizeof(editor->status), "READ mode - press w to edit");
        return 0;
    }
    if (editor->search.query == NULL || editor->search.query_length == 0) return 0;

    DocumentRange full_range = {{0, 0}, document_end_position(&editor->document)};
    size_t source_length = 0;
    char *source = document_extract_range_with_length(&editor->document, full_range, &source_length);
    if (source == NULL) return 0;
    int replace_count = 0;
    size_t replaced_length = 0;
    char *replaced = replace_all_bytes(source, source_length, editor->search.query, replacement, &replace_count, &replaced_length);
    free(source);
    if (replaced == NULL || replace_count == 0) {
        free(replaced);
        snprintf(editor->status, sizeof(editor->status), "No match");
        return 0;
    }

    bool ok = editor_apply_replace(editor, full_range, replaced, replaced_length, false);
    free(replaced);
    if (!ok) return 0;
    editor->search.has_current = false;
    editor->search.match_count = search_count_matches(&editor->document, &editor->search);
    snprintf(editor->status, sizeof(editor->status), "Replaced %d matches", replace_count);
    return replace_count;
}

static void editor_finish_prompt(Editor *editor) {
    if (editor->prompt_mode == EDITOR_PROMPT_FIND) {
        if (editor_search_set_query(editor, editor->prompt_buffer)) {
            editor_find_next(editor);
        }
        editor->prompt_mode = EDITOR_PROMPT_NONE;
        return;
    }
    if (editor->prompt_mode == EDITOR_PROMPT_REPLACE_CURRENT) {
        editor_replace_current(editor, editor->prompt_buffer);
        editor->prompt_mode = EDITOR_PROMPT_NONE;
        return;
    }
    if (editor->prompt_mode == EDITOR_PROMPT_REPLACE_ALL) {
        snprintf(editor->replacement_buffer, sizeof(editor->replacement_buffer), "%s", editor->prompt_buffer);
        editor->prompt_buffer[0] = '\0';
        editor->prompt_mode = EDITOR_PROMPT_CONFIRM_REPLACE_ALL;
        snprintf(editor->status, sizeof(editor->status), "Replace all? y/n");
        return;
    }
}

static void editor_handle_prompt_key(Editor *editor, int key) {
    if (editor->prompt_mode == EDITOR_PROMPT_CONFIRM_REPLACE_ALL) {
        if (key == 'y' || key == 'Y') {
            editor_replace_all_confirmed(editor, editor->replacement_buffer);
        } else {
            snprintf(editor->status, sizeof(editor->status), "Replace all cancelled");
        }
        editor->prompt_mode = EDITOR_PROMPT_NONE;
        return;
    }

    if (key == '\x1b') {
        editor->prompt_mode = EDITOR_PROMPT_NONE;
        snprintf(editor->status, sizeof(editor->status), "Prompt cancelled");
        return;
    }
    if (key == TEDIT_KEY_BACKSPACE) {
        size_t length = strlen(editor->prompt_buffer);
        if (length > 0) editor->prompt_buffer[length - 1] = '\0';
        return;
    }
    if (key == '\r' || key == '\n') {
        editor_finish_prompt(editor);
        return;
    }
    if (key >= 32 && key <= 126) {
        size_t length = strlen(editor->prompt_buffer);
        if (length + 1 < sizeof(editor->prompt_buffer)) {
            editor->prompt_buffer[length] = (char)key;
            editor->prompt_buffer[length + 1] = '\0';
        }
    }
}

void editor_handle_key(Editor *editor, int key) {
    if (key == TEDIT_KEY_CTRL_Q) {
        editor->should_quit = 1;
        return;
    }

    if (editor->prompt_mode != EDITOR_PROMPT_NONE) {
        editor_handle_prompt_key(editor, key);
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

    if (key == TEDIT_KEY_CTRL_F) {
        editor_start_prompt(editor, EDITOR_PROMPT_FIND, "Find:");
        return;
    }

    if (key == TEDIT_KEY_CTRL_G) {
        editor_find_next(editor);
        return;
    }

    if (key == TEDIT_KEY_CTRL_P) {
        editor_find_previous(editor);
        return;
    }

    if (key == TEDIT_KEY_CTRL_R || key == TEDIT_KEY_CTRL_A) {
        if (editor->mode == EDITOR_MODE_READ) {
            snprintf(editor->status, sizeof(editor->status), "READ mode - press w to edit");
            return;
        }
        editor_start_prompt(editor,
                            key == TEDIT_KEY_CTRL_R ? EDITOR_PROMPT_REPLACE_CURRENT : EDITOR_PROMPT_REPLACE_ALL,
                            key == TEDIT_KEY_CTRL_R ? "Replace current with:" : "Replace all with:");
        return;
    }

    if (key == TEDIT_KEY_CTRL_Z || key == TEDIT_KEY_CTRL_Y) {
        if (editor->mode == EDITOR_MODE_READ) {
            snprintf(editor->status, sizeof(editor->status), "READ mode - press w to edit");
            return;
        }
        if (key == TEDIT_KEY_CTRL_Z) editor_undo(editor);
        else editor_redo(editor);
        return;
    }

    if (key >= TEDIT_KEY_SHIFT_ARROW_LEFT && key <= TEDIT_KEY_SHIFT_ARROW_DOWN) {
        editor_extend_selection(editor, key);
        return;
    }

    if ((key >= TEDIT_KEY_ARROW_LEFT && key <= TEDIT_KEY_ARROW_DOWN) ||
        (key >= TEDIT_KEY_HOME && key <= TEDIT_KEY_CTRL_END)) {
        move_cursor(editor, key);
        editor_clear_selection(editor);
        return;
    }

    if (key == TEDIT_KEY_CTRL_C) {
        editor_copy_selection(editor);
        return;
    }

    if (key == TEDIT_KEY_CTRL_X || key == TEDIT_KEY_CTRL_V || key == TEDIT_KEY_CTRL_K) {
        if (editor->mode == EDITOR_MODE_READ) {
            snprintf(editor->status, sizeof(editor->status), "READ mode - press w to edit");
            return;
        }
        if (key == TEDIT_KEY_CTRL_X) editor_cut_selection(editor);
        else if (key == TEDIT_KEY_CTRL_V) editor_paste_clipboard(editor);
        else editor_cut_line(editor);
        return;
    }

    if (editor->mode == EDITOR_MODE_READ && editor_is_editing_key(key)) {
        snprintf(editor->status, sizeof(editor->status), "READ mode - press w to edit");
        return;
    }

    if (key == TEDIT_KEY_BACKSPACE) {
        editor_delete_backward(editor);
        return;
    }

    if (key == TEDIT_KEY_DELETE) {
        editor_delete_forward(editor);
        return;
    }

    if (key == '\r' || key == '\n') {
        editor_insert_text(editor, "\n", 1, false);
        return;
    }

    if (key == '\t') {
        editor_insert_text(editor, "\t", 1, false);
        return;
    }

    if (key >= 32 && key <= 126) {
        char ch = (char)key;
        editor_insert_text(editor, &ch, 1, true);
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
