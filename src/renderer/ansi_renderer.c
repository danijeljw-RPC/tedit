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

static void write_syntax_style(Platform *platform, SyntaxTokenType type) {
    switch (type) {
        case SYNTAX_TOKEN_KEYWORD:
        case SYNTAX_TOKEN_TYPE:
            write_str(platform, "\x1b[36m");
            break;
        case SYNTAX_TOKEN_STRING:
        case SYNTAX_TOKEN_CHARACTER:
            write_str(platform, "\x1b[32m");
            break;
        case SYNTAX_TOKEN_COMMENT:
        case SYNTAX_TOKEN_TODO:
            write_str(platform, "\x1b[90m");
            break;
        case SYNTAX_TOKEN_NUMBER:
            write_str(platform, "\x1b[35m");
            break;
        case SYNTAX_TOKEN_PREPROCESSOR:
            write_str(platform, "\x1b[33m");
            break;
        default:
            write_str(platform, "\x1b[m");
            break;
    }
}

static int line_number_width(size_t line_count) {
    int width = 1;
    while (line_count >= 10) {
        line_count /= 10;
        width++;
    }
    return width + 2;
}

static void write_line_prefix(Platform *platform, const Editor *editor, size_t row, int gutter_width) {
    char buf[32];
    if (!editor->settings.show_line_numbers) return;
    snprintf(buf, sizeof(buf), "%*zu ", gutter_width - 1, row + 1);
    platform_write(platform, buf, (int)strlen(buf));
}

static void write_display_byte(Platform *platform, const Editor *editor, const char *byte) {
    if (editor->settings.show_whitespace && *byte == ' ') {
        write_str(platform, ".");
    } else if (editor->settings.show_whitespace && *byte == '\t') {
        write_str(platform, ">");
    } else {
        platform_write(platform, byte, 1);
    }
}

static void write_line_slice(Platform *platform, const Editor *editor, size_t row, const TextLine *line, size_t left, int width) {
    if (width <= 0) return;
    if (left >= line->length) return;
    size_t available = line->length - left;
    size_t count = available < (size_t)width ? available : (size_t)width;
    EditorHighlight current = TEDIT_HIGHLIGHT_NONE;
    SyntaxTokenType current_syntax = SYNTAX_TOKEN_NORMAL;
    SyntaxTokenLine syntax_tokens;
    syntax_token_line_init(&syntax_tokens);
    syntax_highlight_line(&editor->syntax, line->data == NULL ? "" : line->data, line->length, &syntax_tokens);
    for (size_t i = 0; i < count; i++) {
        size_t col = left + i;
        EditorHighlight next = editor_highlight_at(editor, row, col);
        if (next != current) {
            write_highlight_style(platform, next);
            current = next;
            current_syntax = SYNTAX_TOKEN_NORMAL;
        }
        if (next == TEDIT_HIGHLIGHT_NONE) {
            SyntaxTokenType syntax = syntax_token_line_type_at(&syntax_tokens, col);
            if (syntax != current_syntax) {
                write_syntax_style(platform, syntax);
                current_syntax = syntax;
            }
        }
        write_display_byte(platform, editor, line->data + col);
    }
    if (current != TEDIT_HIGHLIGHT_NONE || current_syntax != SYNTAX_TOKEN_NORMAL) write_highlight_style(platform, TEDIT_HIGHLIGHT_NONE);
    syntax_token_line_destroy(&syntax_tokens);
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

    write_str(platform, "\x1b[7m");
    write_str(platform, " ");
    for (int i = 0; i < MENU_COUNT; i++) {
        if (editor->menu_bar.open && editor->menu_bar.active_menu == (MenuId)i) write_str(platform, "\x1b[m\x1b[7m[");
        snprintf(buf, sizeof(buf), "%s", menu_bar_menu_label(&editor->menu_bar, (MenuId)i));
        platform_write(platform, buf, (int)strlen(buf));
        if (editor->menu_bar.open && editor->menu_bar.active_menu == (MenuId)i) write_str(platform, "]");
        write_str(platform, " ");
    }
    int menu_len = 1;
    for (int i = 0; i < MENU_COUNT; i++) menu_len += (int)strlen(menu_bar_menu_label(&editor->menu_bar, (MenuId)i)) + 1;
    if (editor->menu_bar.open) menu_len += 2;
    for (int i = menu_len; i < cols; i++) write_str(platform, " ");
    write_str(platform, "\x1b[m\r\n");

    int menu_rows = 1;
    if (editor->menu_bar.open) {
        write_str(platform, "\x1b[K");
        size_t count = menu_bar_item_count(editor->menu_bar.active_menu);
        for (size_t i = 0; i < count; i++) {
            const MenuItem *item = menu_bar_item(editor->menu_bar.active_menu, i);
            if (i == editor->menu_bar.active_item) write_str(platform, "\x1b[7m");
            snprintf(buf, sizeof(buf), " %s ", item == NULL ? "" : item->label);
            platform_write(platform, buf, (int)strlen(buf));
            if (i == editor->menu_bar.active_item) write_str(platform, "\x1b[m");
        }
        write_str(platform, "\r\n");
        menu_rows = 2;
    }

    int usable_rows = rows - 2 - menu_rows;
    if (usable_rows < 1) usable_rows = 1;
    int gutter_width = editor->settings.show_line_numbers ? line_number_width(editor->document.line_count) : 0;
    int text_width = cols - gutter_width;
    for (int screen_row = 0; screen_row < usable_rows; screen_row++) {
        size_t doc_row = editor->viewport.top_row + (size_t)screen_row;
        write_str(platform, "\x1b[K");

        if (doc_row < editor->document.line_count) {
            const TextLine *line = &editor->document.lines[doc_row];
            size_t left = doc_row == editor->cursor.row ? editor->viewport.active_line_left_col : 0;
            write_line_prefix(platform, editor, doc_row, gutter_width);
            write_line_slice(platform, editor, doc_row, line, left, text_width);
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

    int cursor_screen_row = (int)(editor->cursor.row - editor->viewport.top_row) + menu_rows + 1;
    int cursor_screen_col = (int)(editor->cursor.col - editor->viewport.active_line_left_col) + gutter_width + 1;
    if (cursor_screen_col < 1) cursor_screen_col = 1;
    if (cursor_screen_col > cols) cursor_screen_col = cols;
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cursor_screen_row, cursor_screen_col);
    write_str(platform, buf);
    write_str(platform, "\x1b[?25h");
}
