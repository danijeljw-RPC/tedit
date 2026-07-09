#ifndef TEDIT_EDITOR_H
#define TEDIT_EDITOR_H

#include "document.h"
#include "search.h"
#include "syntax.h"
#include "undo.h"
#include "ui/file_browser.h"
#include "ui/menu.h"

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

typedef enum {
    TEDIT_HIGHLIGHT_NONE,
    TEDIT_HIGHLIGHT_SEARCH,
    TEDIT_HIGHLIGHT_SELECTION
} EditorHighlight;

typedef enum {
    EDITOR_PROMPT_NONE,
    EDITOR_PROMPT_FIND,
    EDITOR_PROMPT_REPLACE_CURRENT,
    EDITOR_PROMPT_REPLACE_ALL,
    EDITOR_PROMPT_CONFIRM_REPLACE_ALL,
    EDITOR_PROMPT_OPEN,
    EDITOR_PROMPT_SAVE_AS
} EditorPromptMode;

typedef enum {
    EDITOR_TAB_LITERAL,
    EDITOR_TAB_TWO_SPACES,
    EDITOR_TAB_FOUR_SPACES
} EditorTabMode;

typedef struct {
    bool show_line_numbers;
    bool show_whitespace;
    EditorTabMode tab_mode;
} EditorSettings;

typedef struct {
    bool open;
} AboutDialog;

typedef struct {
    bool active;
    DocumentPosition anchor;
    DocumentPosition cursor;
} Selection;

typedef struct {
    Document document;
    Cursor cursor;
    Viewport viewport;
    UndoStack undo;
    SearchState search;
    Selection selection;
    MenuBar menu_bar;
    FileBrowser file_browser;
    SyntaxDefinition syntax;
    SyntaxTokenLine syntax_scratch;
    EditorSettings settings;
    AboutDialog about_dialog;
    bool menu_alt_prefix_pending;
    bool clear_screen_on_exit;
    char *clipboard;
    size_t clipboard_length;
    EditorPromptMode prompt_mode;
    char prompt_buffer[256];
    char replacement_buffer[256];
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
int editor_search_set_query(Editor *editor, const char *query);
int editor_find_next(Editor *editor);
int editor_find_previous(Editor *editor);
int editor_replace_current(Editor *editor, const char *replacement);
int editor_replace_all_confirmed(Editor *editor, const char *replacement);
void editor_set_selection(Editor *editor, DocumentPosition anchor, DocumentPosition cursor);
void editor_clear_selection(Editor *editor);
int editor_has_selection(const Editor *editor);
const char *editor_clipboard_text(const Editor *editor);
EditorHighlight editor_highlight_at(const Editor *editor, size_t row, size_t col);
void editor_start_open_prompt(Editor *editor);
void editor_start_save_as_prompt(Editor *editor);
int editor_save_as_path(Editor *editor, const char *path);
void editor_toggle_line_numbers(Editor *editor);
void editor_toggle_whitespace(Editor *editor);
void editor_set_tab_mode(Editor *editor, EditorTabMode mode);
int editor_menu_is_open(const Editor *editor);
MenuId editor_active_menu(const Editor *editor);
SyntaxTokenType editor_syntax_token_at(Editor *editor, size_t row, size_t col);
int editor_about_dialog_is_open(const Editor *editor);
const char *editor_about_dialog_title(const Editor *editor);
const char *editor_about_dialog_copyright(const Editor *editor);
int editor_should_clear_screen_on_exit(const Editor *editor);
int editor_prompt_dialog_is_open(const Editor *editor);
const char *editor_prompt_dialog_title(const Editor *editor);
const char *editor_prompt_dialog_value(const Editor *editor);

#endif
