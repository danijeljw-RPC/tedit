#include "core/document.h"
#include "core/editor.h"
#include "platform/platform.h"
#include "renderer/screen_buffer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

static void write_file_bytes(const char *path, const char *data, size_t length) {
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    assert(fwrite(data, 1, length, file) == length);
    assert(fclose(file) == 0);
}

static void read_file_bytes(const char *path, char *buffer, size_t capacity, size_t *length) {
    FILE *file = fopen(path, "rb");
    assert(file != NULL);
    *length = fread(buffer, 1, capacity, file);
    assert(fclose(file) == 0);
}

static void test_basic_insert_delete(void) {
    Document doc;
    document_init(&doc);
    assert(doc.line_count == 1);
    document_insert_char(&doc, 0, 0, 'A');
    assert(doc.lines[0].length == 1);
    size_t row = 99;
    size_t col = 99;
    document_delete_char_before(&doc, 0, 1, &row, &col);
    assert(doc.lines[0].length == 0);
    assert(row == 0);
    assert(col == 0);
    document_destroy(&doc);
}

static void test_insert_newline_splits_line(void) {
    Document doc;
    document_init(&doc);
    document_insert_char(&doc, 0, 0, 'A');
    document_insert_char(&doc, 0, 1, 'B');
    document_insert_char(&doc, 0, 2, 'C');

    size_t row = 0;
    size_t col = 0;
    document_insert_newline(&doc, 0, 1, &row, &col);

    assert(doc.line_count == 2);
    assert(strcmp(doc.lines[0].data, "A") == 0);
    assert(strcmp(doc.lines[1].data, "BC") == 0);
    assert(row == 1);
    assert(col == 0);
    document_destroy(&doc);
}

static void test_backspace_at_line_start_joins_previous_line(void) {
    Document doc;
    document_init(&doc);
    document_insert_char(&doc, 0, 0, 'A');
    size_t row = 0;
    size_t col = 0;
    document_insert_newline(&doc, 0, 1, &row, &col);
    document_insert_char(&doc, 1, 0, 'B');

    document_delete_char_before(&doc, 1, 0, &row, &col);

    assert(doc.line_count == 1);
    assert(strcmp(doc.lines[0].data, "AB") == 0);
    assert(row == 0);
    assert(col == 1);
    document_destroy(&doc);
}

static void test_delete_at_line_end_joins_next_line(void) {
    Document doc;
    document_init(&doc);
    document_insert_char(&doc, 0, 0, 'A');
    size_t row = 0;
    size_t col = 0;
    document_insert_newline(&doc, 0, 1, &row, &col);
    document_insert_char(&doc, 1, 0, 'B');

    document_delete_char_at(&doc, 0, 1);

    assert(doc.line_count == 1);
    assert(strcmp(doc.lines[0].data, "AB") == 0);
    document_destroy(&doc);
}

static void test_missing_path_loads_new_document(void) {
    const char *path = "tedit_test_missing.tmp";
    remove(path);

    Document doc;
    bool created_new = false;
    document_init(&doc);

    assert(document_load_path(&doc, path, &created_new));
    assert(created_new == true);
    assert(doc.path != NULL);
    assert(strcmp(doc.path, path) == 0);
    assert(doc.line_count == 1);
    assert(doc.lines[0].length == 0);
    assert(doc.dirty == false);

    document_destroy(&doc);
    remove(path);
}

static void test_crlf_line_endings_are_preserved_on_save(void) {
    const char *path = "tedit_test_crlf.tmp";
    const char *contents = "alpha\r\nbeta\r\n";
    char buffer[64];
    size_t length = 0;
    bool created_new = true;

    write_file_bytes(path, contents, strlen(contents));

    Document doc;
    document_init(&doc);
    assert(document_load_path(&doc, path, &created_new));
    assert(created_new == false);
    assert(doc.line_ending == DOCUMENT_LINE_ENDING_CRLF);
    assert(document_save(&doc));
    document_destroy(&doc);

    read_file_bytes(path, buffer, sizeof(buffer), &length);
    assert(length == strlen(contents));
    assert(memcmp(buffer, contents, length) == 0);
    remove(path);
}

#ifndef _WIN32
static void test_save_preserves_existing_file_permissions(void) {
    const char *path = "tedit_test_permissions.tmp";
    write_file_bytes(path, "alpha\n", 6);
    assert(chmod(path, 0600) == 0);

    Document doc;
    bool created_new = true;
    struct stat st;
    document_init(&doc);
    assert(document_load_path(&doc, path, &created_new));
    assert(created_new == false);
    document_insert_char(&doc, 0, 5, '!');
    assert(document_save(&doc));
    assert(stat(path, &st) == 0);
    assert((st.st_mode & 0777) == 0600);
    document_destroy(&doc);
    remove(path);
}
#endif

static void test_screen_buffer_resize(void) {
    ScreenBuffer buffer;
    screen_buffer_init(&buffer);
    assert(screen_buffer_resize(&buffer, 10, 20) == 1);
    screen_buffer_destroy(&buffer);
}

static void test_read_mode_blocks_printable_insert(void) {
    Editor editor;
    editor_init(&editor);
    editor.mode = EDITOR_MODE_READ;

    editor_handle_key(&editor, 'A');

    assert(editor.document.lines[0].length == 0);
    assert(editor.document.dirty == false);
    editor_destroy(&editor);
}

static void test_w_enters_write_mode(void) {
    Editor editor;
    editor_init(&editor);
    assert(editor.mode == EDITOR_MODE_READ);

    editor_handle_key(&editor, 'w');

    assert(editor.mode == EDITOR_MODE_WRITE);
    editor_destroy(&editor);
}

static void test_write_mode_allows_printable_insert(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);

    editor_handle_key(&editor, 'A');

    assert(strcmp(editor.document.lines[0].data, "A") == 0);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 1);
    assert(editor.document.dirty == true);
    editor_destroy(&editor);
}

static void test_editor_backspace_newline_delete_and_quit(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);

    editor_handle_key(&editor, 'A');
    editor_handle_key(&editor, '\r');
    editor_handle_key(&editor, 'B');
    assert(editor.document.line_count == 2);
    assert(strcmp(editor.document.lines[0].data, "A") == 0);
    assert(strcmp(editor.document.lines[1].data, "B") == 0);
    assert(editor.cursor.row == 1);
    assert(editor.cursor.col == 1);

    editor_handle_key(&editor, TEDIT_KEY_BACKSPACE);
    assert(editor.document.line_count == 2);
    assert(strcmp(editor.document.lines[1].data, "") == 0);
    assert(editor.cursor.row == 1);
    assert(editor.cursor.col == 0);

    editor_handle_key(&editor, TEDIT_KEY_BACKSPACE);
    assert(editor.document.line_count == 1);
    assert(strcmp(editor.document.lines[0].data, "A") == 0);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 1);

    editor_handle_key(&editor, 'B');
    editor.cursor.col = 1;
    editor_handle_key(&editor, TEDIT_KEY_DELETE);
    assert(strcmp(editor.document.lines[0].data, "A") == 0);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_Q);
    assert(editor.should_quit == 1);
    editor_destroy(&editor);
}

static void test_editor_open_existing_file_starts_read_mode(void) {
    const char *path = "tedit_test_existing_open.tmp";
    write_file_bytes(path, "hello\n", 6);

    Editor editor;
    editor_init(&editor);

    assert(editor_open(&editor, path) == 1);
    assert(editor.mode == EDITOR_MODE_READ);
    assert(editor.document.path != NULL);
    assert(strcmp(editor.document.path, path) == 0);
    assert(editor.document.dirty == false);
    assert(strcmp(editor.document.lines[0].data, "hello") == 0);

    editor_destroy(&editor);
    remove(path);
}

static void test_editor_open_missing_file_starts_write_mode(void) {
    const char *path = "tedit_test_missing_open.tmp";
    remove(path);

    Editor editor;
    editor_init(&editor);

    assert(editor_open(&editor, path) == 1);
    assert(editor.mode == EDITOR_MODE_WRITE);
    assert(editor.document.path != NULL);
    assert(strcmp(editor.document.path, path) == 0);
    assert(editor.document.dirty == false);
    assert(editor.document.line_count == 1);
    assert(editor.document.lines[0].length == 0);

    editor_destroy(&editor);
    remove(path);
}

static void test_active_line_horizontal_scroll_resets_when_row_changes(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    for (size_t i = 0; i < 12; i++) {
        document_insert_char(&editor.document, 0, i, (char)('a' + i));
    }
    size_t row = 0;
    size_t col = 0;
    document_insert_newline(&editor.document, 0, 12, &row, &col);

    editor.screen_rows = 5;
    editor.screen_cols = 5;
    editor.cursor.row = 0;
    editor.cursor.col = 11;
    editor_update_viewport(&editor);
    assert(editor.viewport.active_line_left_col > 0);

    editor.cursor.row = 1;
    editor.cursor.col = 0;
    editor_update_viewport(&editor);
    assert(editor.viewport.active_line_left_col == 0);

    editor_destroy(&editor);
}

static void test_home_end_move_within_line(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    editor_handle_key(&editor, 'A');
    editor_handle_key(&editor, 'B');
    editor_handle_key(&editor, 'C');

    editor_handle_key(&editor, TEDIT_KEY_HOME);
    assert(editor.cursor.col == 0);
    editor_handle_key(&editor, TEDIT_KEY_END);
    assert(editor.cursor.col == 3);
    editor_destroy(&editor);
}

static void test_page_and_document_navigation_keys(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);

    for (int i = 0; i < 9; i++) {
        editor_handle_key(&editor, (int)('a' + i));
        if (i < 8) editor_handle_key(&editor, '\n');
    }
    editor.screen_rows = 5;
    editor.screen_cols = 20;
    editor.cursor.row = 0;
    editor.cursor.col = 0;

    editor_handle_key(&editor, TEDIT_KEY_PAGE_DOWN);
    assert(editor.cursor.row == 3);
    editor_handle_key(&editor, TEDIT_KEY_PAGE_UP);
    assert(editor.cursor.row == 0);
    editor_handle_key(&editor, TEDIT_KEY_CTRL_END);
    assert(editor.cursor.row == editor.document.line_count - 1);
    assert(editor.cursor.col == editor.document.lines[editor.cursor.row].length);
    editor_handle_key(&editor, TEDIT_KEY_CTRL_HOME);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 0);
    editor_destroy(&editor);
}

static void test_escape_sequence_navigation_mapping(void) {
    assert(platform_key_from_escape_sequence("[H") == TEDIT_KEY_HOME);
    assert(platform_key_from_escape_sequence("[F") == TEDIT_KEY_END);
    assert(platform_key_from_escape_sequence("[5~") == TEDIT_KEY_PAGE_UP);
    assert(platform_key_from_escape_sequence("[6~") == TEDIT_KEY_PAGE_DOWN);
    assert(platform_key_from_escape_sequence("[1;5H") == TEDIT_KEY_CTRL_HOME);
    assert(platform_key_from_escape_sequence("[1;5F") == TEDIT_KEY_CTRL_END);
    assert(platform_key_from_escape_sequence("[3~") == TEDIT_KEY_DELETE);
    assert(platform_key_from_escape_sequence("[unknown") == '\x1b');
}

int main(void) {
    test_basic_insert_delete();
    test_insert_newline_splits_line();
    test_backspace_at_line_start_joins_previous_line();
    test_delete_at_line_end_joins_next_line();
    test_missing_path_loads_new_document();
    test_crlf_line_endings_are_preserved_on_save();
#ifndef _WIN32
    test_save_preserves_existing_file_permissions();
#endif
    test_screen_buffer_resize();
    test_read_mode_blocks_printable_insert();
    test_w_enters_write_mode();
    test_write_mode_allows_printable_insert();
    test_editor_backspace_newline_delete_and_quit();
    test_editor_open_existing_file_starts_read_mode();
    test_editor_open_missing_file_starts_write_mode();
    test_active_line_horizontal_scroll_resets_when_row_changes();
    test_home_end_move_within_line();
    test_page_and_document_navigation_keys();
    test_escape_sequence_navigation_mapping();

    return 0;
}
