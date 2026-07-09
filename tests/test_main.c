#include "core/document.h"
#include "core/editor.h"
#include "platform/platform.h"
#include "renderer/screen_buffer.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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

static void test_document_extract_delete_and_replace_range(void) {
    Document doc;
    document_init(&doc);
    document_insert_char(&doc, 0, 0, 'a');
    document_insert_char(&doc, 0, 1, 'b');
    document_insert_char(&doc, 0, 2, 'c');
    size_t row = 0;
    size_t col = 0;
    document_insert_newline(&doc, 0, 3, &row, &col);
    document_insert_char(&doc, 1, 0, 'd');
    document_insert_char(&doc, 1, 1, 'e');
    document_insert_char(&doc, 1, 2, 'f');

    DocumentRange range = {{0, 1}, {1, 2}};
    char *text = document_extract_range(&doc, range);
    assert(text != NULL);
    assert(strcmp(text, "bc\nde") == 0);
    free(text);

    DocumentPosition cursor = document_delete_range(&doc, range);
    assert(doc.line_count == 1);
    assert(strcmp(doc.lines[0].data, "af") == 0);
    assert(cursor.row == 0);
    assert(cursor.col == 1);

    DocumentRange replace_range = {{0, 1}, {0, 2}};
    cursor = document_replace_range(&doc, replace_range, "XY\nZ", 4);
    assert(doc.line_count == 2);
    assert(strcmp(doc.lines[0].data, "aXY") == 0);
    assert(strcmp(doc.lines[1].data, "Z") == 0);
    assert(cursor.row == 1);
    assert(cursor.col == 1);
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

static void test_grouped_printable_insert_undoes_and_redoes_as_one_edit(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);

    editor_handle_key(&editor, 'h');
    editor_handle_key(&editor, 'e');
    editor_handle_key(&editor, 'l');
    editor_handle_key(&editor, 'l');
    editor_handle_key(&editor, 'o');
    assert(strcmp(editor.document.lines[0].data, "hello") == 0);
    assert(editor.cursor.col == 5);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_Z);
    assert(strcmp(editor.document.lines[0].data, "") == 0);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 0);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_Y);
    assert(strcmp(editor.document.lines[0].data, "hello") == 0);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 5);
    editor_destroy(&editor);
}

static void test_redo_stack_clears_after_new_edit(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);

    editor_handle_key(&editor, 'a');
    editor_handle_key(&editor, 'b');
    editor_handle_key(&editor, 'c');
    editor_handle_key(&editor, TEDIT_KEY_CTRL_Z);
    assert(strcmp(editor.document.lines[0].data, "") == 0);

    editor_handle_key(&editor, 'x');
    editor_handle_key(&editor, TEDIT_KEY_CTRL_Y);

    assert(strcmp(editor.document.lines[0].data, "x") == 0);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 1);
    editor_destroy(&editor);
}

static void test_undo_preserves_deleted_nul_byte(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char bytes[] = {'a', '\0', 'b'};
    document_insert_bytes(&editor.document, (DocumentPosition){0, 0}, bytes, sizeof(bytes));
    editor.cursor.row = 0;
    editor.cursor.col = 2;

    editor_handle_key(&editor, TEDIT_KEY_BACKSPACE);
    assert(editor.document.lines[0].length == 2);
    assert(editor.document.lines[0].data[0] == 'a');
    assert(editor.document.lines[0].data[1] == 'b');

    editor_handle_key(&editor, TEDIT_KEY_CTRL_Z);
    assert(editor.document.lines[0].length == 3);
    assert(memcmp(editor.document.lines[0].data, bytes, sizeof(bytes)) == 0);
    editor_destroy(&editor);
}

static void test_undo_redo_restores_newline_edit(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);

    editor_handle_key(&editor, 'A');
    editor_handle_key(&editor, '\n');
    editor_handle_key(&editor, 'B');
    assert(editor.document.line_count == 2);
    assert(strcmp(editor.document.lines[0].data, "A") == 0);
    assert(strcmp(editor.document.lines[1].data, "B") == 0);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_Z);
    assert(editor.document.line_count == 2);
    assert(strcmp(editor.document.lines[0].data, "A") == 0);
    assert(strcmp(editor.document.lines[1].data, "") == 0);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_Z);
    assert(editor.document.line_count == 1);
    assert(strcmp(editor.document.lines[0].data, "A") == 0);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 1);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_Y);
    assert(editor.document.line_count == 2);
    assert(strcmp(editor.document.lines[0].data, "A") == 0);
    assert(strcmp(editor.document.lines[1].data, "") == 0);
    assert(editor.cursor.row == 1);
    assert(editor.cursor.col == 0);
    editor_destroy(&editor);
}

static void test_search_find_next_previous_wraps(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char *text = "one two one";
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);
    editor.cursor.row = 0;
    editor.cursor.col = 0;

    assert(editor_search_set_query(&editor, "one") == 1);
    assert(editor_find_next(&editor) == 1);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 0);
    assert(editor.search.current.start.row == 0);
    assert(editor.search.current.start.col == 0);

    assert(editor_find_next(&editor) == 1);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 8);

    assert(editor_find_next(&editor) == 1);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 0);

    assert(editor_find_previous(&editor) == 1);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 8);
    editor_destroy(&editor);
}

static void test_search_previous_wraps_to_single_match_at_column_zero(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    editor_handle_key(&editor, 'o');
    editor_handle_key(&editor, 'n');
    editor_handle_key(&editor, 'e');
    editor.cursor.row = 0;
    editor.cursor.col = 0;

    assert(editor_search_set_query(&editor, "one") == 1);
    assert(editor_find_next(&editor) == 1);
    assert(editor_find_previous(&editor) == 1);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 0);
    editor_destroy(&editor);
}

static void test_replace_current_records_one_undoable_edit(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char *text = "cat dog cat";
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);
    editor.cursor.row = 0;
    editor.cursor.col = 0;

    assert(editor_search_set_query(&editor, "cat") == 1);
    assert(editor_find_next(&editor) == 1);
    assert(editor_replace_current(&editor, "fox") == 1);
    assert(strcmp(editor.document.lines[0].data, "fox dog cat") == 0);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_Z);
    assert(strcmp(editor.document.lines[0].data, "cat dog cat") == 0);
    editor_destroy(&editor);
}

static void test_replace_current_recomputes_after_document_mutation(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char *text = "cat";
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);
    editor.cursor.row = 0;
    editor.cursor.col = 0;
    assert(editor_search_set_query(&editor, "cat") == 1);
    assert(editor_find_next(&editor) == 1);

    editor.cursor.row = 0;
    editor.cursor.col = 0;
    editor_handle_key(&editor, 'X');
    assert(editor_replace_current(&editor, "fox") == 1);
    assert(strcmp(editor.document.lines[0].data, "Xfox") == 0);
    editor_destroy(&editor);
}

static void test_replace_all_is_single_undo_group(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char *text = "cat dog cat";
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);
    editor_handle_key(&editor, '\n');
    const char *line2 = "cat";
    for (size_t i = 0; line2[i] != '\0'; i++) editor_handle_key(&editor, line2[i]);

    assert(editor_search_set_query(&editor, "cat") == 1);
    assert(editor_replace_all_confirmed(&editor, "fox") == 3);
    assert(strcmp(editor.document.lines[0].data, "fox dog fox") == 0);
    assert(strcmp(editor.document.lines[1].data, "fox") == 0);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_Z);
    assert(editor.document.line_count == 2);
    assert(strcmp(editor.document.lines[0].data, "cat dog cat") == 0);
    assert(strcmp(editor.document.lines[1].data, "cat") == 0);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_Y);
    assert(strcmp(editor.document.lines[0].data, "fox dog fox") == 0);
    assert(strcmp(editor.document.lines[1].data, "fox") == 0);
    editor_destroy(&editor);
}

static void test_read_mode_blocks_replace_mutations(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char *text = "cat cat";
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);
    editor.mode = EDITOR_MODE_READ;
    editor.cursor.row = 0;
    editor.cursor.col = 0;

    assert(editor_search_set_query(&editor, "cat") == 1);
    assert(editor_find_next(&editor) == 1);
    assert(editor_replace_current(&editor, "fox") == 0);
    assert(editor_replace_all_confirmed(&editor, "fox") == 0);
    assert(strcmp(editor.document.lines[0].data, "cat cat") == 0);
    editor_destroy(&editor);
}

static void test_selection_delete_and_type_over_selection(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char *text = "hello";
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);

    editor_set_selection(&editor, (DocumentPosition){0, 1}, (DocumentPosition){0, 4});
    editor_handle_key(&editor, TEDIT_KEY_DELETE);
    assert(strcmp(editor.document.lines[0].data, "ho") == 0);
    assert(editor_has_selection(&editor) == 0);
    editor_destroy(&editor);

    editor_init(&editor);
    editor_enter_write_mode(&editor);
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);
    editor_set_selection(&editor, (DocumentPosition){0, 1}, (DocumentPosition){0, 4});
    editor_handle_key(&editor, 'X');
    assert(strcmp(editor.document.lines[0].data, "hXo") == 0);
    assert(editor.cursor.row == 0);
    assert(editor.cursor.col == 2);
    assert(editor_has_selection(&editor) == 0);
    editor_destroy(&editor);
}

static void test_selection_survives_viewport_movement(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    editor_handle_key(&editor, 'a');
    editor_handle_key(&editor, '\n');
    editor_handle_key(&editor, 'b');
    editor_handle_key(&editor, '\n');
    editor_handle_key(&editor, 'c');
    editor_set_selection(&editor, (DocumentPosition){0, 0}, (DocumentPosition){1, 1});

    editor.screen_rows = 3;
    editor.cursor.row = 2;
    editor.cursor.col = 1;
    editor_update_viewport(&editor);

    assert(editor_has_selection(&editor) == 1);
    assert(editor.selection.anchor.row == 0);
    assert(editor.selection.anchor.col == 0);
    assert(editor.selection.cursor.row == 1);
    assert(editor.selection.cursor.col == 1);
    editor_destroy(&editor);
}

static void test_clipboard_copy_cut_paste_and_cut_line(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char *text = "hello";
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);

    editor_set_selection(&editor, (DocumentPosition){0, 1}, (DocumentPosition){0, 4});
    editor.mode = EDITOR_MODE_READ;
    editor_handle_key(&editor, TEDIT_KEY_CTRL_C);
    assert(strcmp(editor_clipboard_text(&editor), "ell") == 0);
    assert(strcmp(editor.document.lines[0].data, "hello") == 0);

    editor_enter_write_mode(&editor);
    editor_handle_key(&editor, TEDIT_KEY_CTRL_X);
    assert(strcmp(editor_clipboard_text(&editor), "ell") == 0);
    assert(strcmp(editor.document.lines[0].data, "ho") == 0);

    editor.cursor.row = 0;
    editor.cursor.col = 1;
    editor_handle_key(&editor, TEDIT_KEY_CTRL_V);
    assert(strcmp(editor.document.lines[0].data, "hello") == 0);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_K);
    assert(strcmp(editor_clipboard_text(&editor), "hello") == 0);
    assert(strcmp(editor.document.lines[0].data, "") == 0);
    editor_destroy(&editor);
}

static void test_cut_paste_preserves_nul_byte(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char bytes[] = {'a', '\0', 'b'};
    document_insert_bytes(&editor.document, (DocumentPosition){0, 0}, bytes, sizeof(bytes));
    editor_set_selection(&editor, (DocumentPosition){0, 0}, (DocumentPosition){0, 3});

    editor_handle_key(&editor, TEDIT_KEY_CTRL_X);
    assert(editor.document.lines[0].length == 0);
    editor_handle_key(&editor, TEDIT_KEY_CTRL_V);
    assert(editor.document.lines[0].length == 3);
    assert(memcmp(editor.document.lines[0].data, bytes, sizeof(bytes)) == 0);
    editor_destroy(&editor);
}

static void test_read_mode_blocks_selection_clipboard_mutations(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char *text = "hello";
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);
    editor_set_selection(&editor, (DocumentPosition){0, 1}, (DocumentPosition){0, 4});
    editor.mode = EDITOR_MODE_READ;

    editor_handle_key(&editor, TEDIT_KEY_DELETE);
    editor_handle_key(&editor, 'X');
    editor_handle_key(&editor, TEDIT_KEY_CTRL_X);
    editor_handle_key(&editor, TEDIT_KEY_CTRL_V);
    editor_handle_key(&editor, TEDIT_KEY_CTRL_K);
    assert(strcmp(editor.document.lines[0].data, "hello") == 0);
    assert(editor.document.dirty == true);
    assert(editor_has_selection(&editor) == 1);
    editor_destroy(&editor);
}

static void test_shift_arrow_extends_selection(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    editor_handle_key(&editor, 'a');
    editor_handle_key(&editor, 'b');
    editor_handle_key(&editor, 'c');
    editor.cursor.row = 0;
    editor.cursor.col = 0;

    editor_handle_key(&editor, TEDIT_KEY_SHIFT_ARROW_RIGHT);
    editor_handle_key(&editor, TEDIT_KEY_SHIFT_ARROW_RIGHT);

    assert(editor_has_selection(&editor) == 1);
    assert(editor.selection.anchor.row == 0);
    assert(editor.selection.anchor.col == 0);
    assert(editor.selection.cursor.row == 0);
    assert(editor.selection.cursor.col == 2);
    editor_destroy(&editor);
}

static void test_highlight_decision_prefers_selection_over_search(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char *text = "cat cat";
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);

    assert(editor_search_set_query(&editor, "cat") == 1);
    assert(editor_highlight_at(&editor, 0, 1) == TEDIT_HIGHLIGHT_SEARCH);
    assert(editor_highlight_at(&editor, 0, 3) == TEDIT_HIGHLIGHT_NONE);
    assert(editor_highlight_at(&editor, 0, 5) == TEDIT_HIGHLIGHT_SEARCH);

    editor_set_selection(&editor, (DocumentPosition){0, 0}, (DocumentPosition){0, 3});
    assert(editor_highlight_at(&editor, 0, 1) == TEDIT_HIGHLIGHT_SELECTION);
    assert(editor_highlight_at(&editor, 0, 5) == TEDIT_HIGHLIGHT_SEARCH);
    editor_destroy(&editor);
}

static void test_find_prompt_and_navigation_keybindings(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char *text = "cat dog cat";
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);
    editor.cursor.row = 0;
    editor.cursor.col = 0;

    editor_handle_key(&editor, TEDIT_KEY_CTRL_F);
    editor_handle_key(&editor, 'c');
    editor_handle_key(&editor, 'a');
    editor_handle_key(&editor, 't');
    editor_handle_key(&editor, '\n');

    assert(editor.search.query != NULL);
    assert(strcmp(editor.search.query, "cat") == 0);
    assert(editor.cursor.col == 0);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_G);
    assert(editor.cursor.col == 8);
    editor_handle_key(&editor, TEDIT_KEY_CTRL_P);
    assert(editor.cursor.col == 0);
    editor_destroy(&editor);
}

static void test_replace_prompt_keybindings(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);
    const char *text = "cat dog cat";
    for (size_t i = 0; text[i] != '\0'; i++) editor_handle_key(&editor, text[i]);
    editor.cursor.row = 0;
    editor.cursor.col = 0;
    assert(editor_search_set_query(&editor, "cat") == 1);
    assert(editor_find_next(&editor) == 1);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_R);
    editor_handle_key(&editor, 'f');
    editor_handle_key(&editor, 'o');
    editor_handle_key(&editor, 'x');
    editor_handle_key(&editor, '\n');
    assert(strcmp(editor.document.lines[0].data, "fox dog cat") == 0);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_A);
    editor_handle_key(&editor, 'f');
    editor_handle_key(&editor, 'o');
    editor_handle_key(&editor, 'x');
    editor_handle_key(&editor, '\n');
    editor_handle_key(&editor, 'y');
    assert(strcmp(editor.document.lines[0].data, "fox dog fox") == 0);
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

static void test_ctrl_s_saves_missing_path_document(void) {
    const char *path = "tedit_test_ctrl_s_missing.tmp";
    char buffer[32];
    size_t length = 0;
    remove(path);

    Editor editor;
    editor_init(&editor);
    assert(editor_open(&editor, path) == 1);
    assert(editor.mode == EDITOR_MODE_WRITE);
    editor_handle_key(&editor, 'O');
    editor_handle_key(&editor, 'K');
    assert(editor.document.dirty == true);

    editor_handle_key(&editor, TEDIT_KEY_CTRL_S);

    assert(editor.document.dirty == false);
    assert(strcmp(editor.status, "Saved") == 0);
    read_file_bytes(path, buffer, sizeof(buffer), &length);
    assert(length == 2);
    assert(memcmp(buffer, "OK", 2) == 0);
    editor_destroy(&editor);
    remove(path);
}

static void test_tab_inserts_literal_tab_in_write_mode(void) {
    Editor editor;
    editor_init(&editor);
    editor_enter_write_mode(&editor);

    editor_handle_key(&editor, '\t');

    assert(editor.document.lines[0].length == 1);
    assert(editor.document.lines[0].data[0] == '\t');
    assert(editor.cursor.col == 1);
    editor_destroy(&editor);
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
    assert(platform_key_from_escape_sequence("[1;2A") == TEDIT_KEY_SHIFT_ARROW_UP);
    assert(platform_key_from_escape_sequence("[1;2B") == TEDIT_KEY_SHIFT_ARROW_DOWN);
    assert(platform_key_from_escape_sequence("[1;2C") == TEDIT_KEY_SHIFT_ARROW_RIGHT);
    assert(platform_key_from_escape_sequence("[1;2D") == TEDIT_KEY_SHIFT_ARROW_LEFT);
    assert(platform_key_from_escape_sequence("[3~") == TEDIT_KEY_DELETE);
    assert(platform_key_from_escape_sequence("q") == TEDIT_KEY_CTRL_Q);
    assert(platform_key_from_escape_sequence("Q") == TEDIT_KEY_CTRL_Q);
    assert(platform_key_from_escape_sequence("[unknown") == '\x1b');
}

int main(void) {
    test_basic_insert_delete();
    test_insert_newline_splits_line();
    test_backspace_at_line_start_joins_previous_line();
    test_delete_at_line_end_joins_next_line();
    test_document_extract_delete_and_replace_range();
    test_missing_path_loads_new_document();
    test_crlf_line_endings_are_preserved_on_save();
#ifndef _WIN32
    test_save_preserves_existing_file_permissions();
#endif
    test_screen_buffer_resize();
    test_read_mode_blocks_printable_insert();
    test_w_enters_write_mode();
    test_write_mode_allows_printable_insert();
    test_grouped_printable_insert_undoes_and_redoes_as_one_edit();
    test_redo_stack_clears_after_new_edit();
    test_undo_preserves_deleted_nul_byte();
    test_undo_redo_restores_newline_edit();
    test_search_find_next_previous_wraps();
    test_search_previous_wraps_to_single_match_at_column_zero();
    test_replace_current_records_one_undoable_edit();
    test_replace_current_recomputes_after_document_mutation();
    test_replace_all_is_single_undo_group();
    test_read_mode_blocks_replace_mutations();
    test_selection_delete_and_type_over_selection();
    test_selection_survives_viewport_movement();
    test_clipboard_copy_cut_paste_and_cut_line();
    test_cut_paste_preserves_nul_byte();
    test_read_mode_blocks_selection_clipboard_mutations();
    test_shift_arrow_extends_selection();
    test_highlight_decision_prefers_selection_over_search();
    test_find_prompt_and_navigation_keybindings();
    test_replace_prompt_keybindings();
    test_editor_backspace_newline_delete_and_quit();
    test_editor_open_existing_file_starts_read_mode();
    test_editor_open_missing_file_starts_write_mode();
    test_ctrl_s_saves_missing_path_document();
    test_tab_inserts_literal_tab_in_write_mode();
    test_active_line_horizontal_scroll_resets_when_row_changes();
    test_home_end_move_within_line();
    test_page_and_document_navigation_keys();
    test_escape_sequence_navigation_mapping();

    return 0;
}
