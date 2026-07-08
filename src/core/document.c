#include "document.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

static char *copy_string(const char *value) {
    if (value == NULL) return NULL;
    size_t len = strlen(value);
    char *copy = malloc(len + 1);
    if (copy == NULL) return NULL;
    memcpy(copy, value, len + 1);
    return copy;
}

static void line_init(TextLine *line) {
    line->data = NULL;
    line->length = 0;
    line->capacity = 0;
}

static void line_destroy(TextLine *line) {
    free(line->data);
    line->data = NULL;
    line->length = 0;
    line->capacity = 0;
}

static bool line_reserve(TextLine *line, size_t capacity) {
    if (capacity <= line->capacity) return true;
    char *next = realloc(line->data, capacity);
    if (next == NULL) return false;
    line->data = next;
    line->capacity = capacity;
    return true;
}

static bool document_reserve_lines(Document *doc, size_t capacity) {
    if (capacity <= doc->capacity) return true;
    TextLine *next = realloc(doc->lines, capacity * sizeof(TextLine));
    if (next == NULL) return false;
    for (size_t i = doc->capacity; i < capacity; i++) {
        line_init(&next[i]);
    }
    doc->lines = next;
    doc->capacity = capacity;
    return true;
}

static bool document_append_line(Document *doc, const char *data, size_t length) {
    if (doc->line_count == doc->capacity) {
        size_t next_capacity = doc->capacity == 0 ? 16 : doc->capacity * 2;
        if (!document_reserve_lines(doc, next_capacity)) return false;
    }

    TextLine *line = &doc->lines[doc->line_count++];
    if (!line_reserve(line, length + 1)) return false;
    memcpy(line->data, data, length);
    line->data[length] = '\0';
    line->length = length;
    return true;
}

static void document_clear_lines(Document *doc) {
    for (size_t i = 0; i < doc->line_count; i++) {
        line_destroy(&doc->lines[i]);
    }
    doc->line_count = 0;
}

static bool document_reset_to_empty(Document *doc) {
    document_clear_lines(doc);
    return document_append_line(doc, "", 0);
}

static bool document_set_path(Document *doc, const char *path) {
    char *copy = copy_string(path);
    if (copy == NULL && path != NULL) return false;
    free(doc->path);
    doc->path = copy;
    return true;
}

static bool document_insert_line(Document *doc, size_t index, const char *data, size_t length) {
    if (index > doc->line_count) return false;
    if (doc->line_count == doc->capacity) {
        size_t next_capacity = doc->capacity == 0 ? 16 : doc->capacity * 2;
        if (!document_reserve_lines(doc, next_capacity)) return false;
    }

    for (size_t i = doc->line_count; i > index; i--) {
        doc->lines[i] = doc->lines[i - 1];
    }
    line_init(&doc->lines[index]);
    doc->line_count++;

    if (!line_reserve(&doc->lines[index], length + 1)) return false;
    memcpy(doc->lines[index].data, data, length);
    doc->lines[index].data[length] = '\0';
    doc->lines[index].length = length;
    return true;
}

static void document_remove_line(Document *doc, size_t index) {
    if (index >= doc->line_count) return;
    line_destroy(&doc->lines[index]);
    for (size_t i = index; i + 1 < doc->line_count; i++) {
        doc->lines[i] = doc->lines[i + 1];
    }
    doc->line_count--;
    if (doc->line_count < doc->capacity) {
        line_init(&doc->lines[doc->line_count]);
    }
}

static bool line_insert_bytes(TextLine *line, size_t col, const char *data, size_t length) {
    if (col > line->length) col = line->length;
    if (!line_reserve(line, line->length + length + 1)) return false;
    memmove(line->data + col + length, line->data + col, line->length - col + 1);
    memcpy(line->data + col, data, length);
    line->length += length;
    return true;
}

static const char *document_line_separator(const Document *doc) {
    return doc->line_ending == DOCUMENT_LINE_ENDING_CRLF ? "\r\n" : "\n";
}

static bool make_temp_path(const char *path, char *buffer, size_t capacity) {
    int written = snprintf(buffer, capacity, "%s.tmp", path);
    return written > 0 && (size_t)written < capacity;
}

static bool replace_file_with_temp(const char *temp_path, const char *path) {
#ifdef _WIN32
    return MoveFileExA(temp_path, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(temp_path, path) == 0;
#endif
}

void document_init(Document *doc) {
    doc->lines = NULL;
    doc->line_count = 0;
    doc->capacity = 0;
    doc->path = NULL;
    doc->dirty = false;
    doc->line_ending = DOCUMENT_LINE_ENDING_LF;
    document_append_line(doc, "", 0);
}

void document_destroy(Document *doc) {
    for (size_t i = 0; i < doc->line_count; i++) {
        line_destroy(&doc->lines[i]);
    }
    free(doc->lines);
    free(doc->path);
}

bool document_open(Document *doc, const char *path) {
    bool created_new = false;
    return document_load_path(doc, path, &created_new) && !created_new;
}

bool document_load_path(Document *doc, const char *path, bool *created_new) {
    if (created_new != NULL) *created_new = false;
    errno = 0;
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        if (errno != ENOENT) return false;
        if (!document_reset_to_empty(doc)) return false;
        if (!document_set_path(doc, path)) return false;
        doc->dirty = false;
        doc->line_ending = DOCUMENT_LINE_ENDING_LF;
        if (created_new != NULL) *created_new = true;
        return true;
    }

    document_clear_lines(doc);
    doc->line_ending = DOCUMENT_LINE_ENDING_LF;

    char buffer[4096];
    TextLine current;
    line_init(&current);
    bool saw_cr = false;

    size_t read_count;
    while ((read_count = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < read_count; i++) {
            char ch = buffer[i];
            if (saw_cr) {
                if (ch == '\n') {
                    doc->line_ending = DOCUMENT_LINE_ENDING_CRLF;
                    document_append_line(doc, current.data == NULL ? "" : current.data, current.length);
                    current.length = 0;
                    if (current.data) current.data[0] = '\0';
                    saw_cr = false;
                    continue;
                }
                if (!line_reserve(&current, current.length + 2)) {
                    fclose(file);
                    line_destroy(&current);
                    return false;
                }
                current.data[current.length++] = '\r';
                current.data[current.length] = '\0';
                saw_cr = false;
            }
            if (ch == '\r') {
                saw_cr = true;
                continue;
            }
            if (ch == '\n') {
                document_append_line(doc, current.data == NULL ? "" : current.data, current.length);
                current.length = 0;
                if (current.data) current.data[0] = '\0';
            } else {
                if (!line_reserve(&current, current.length + 2)) {
                    fclose(file);
                    line_destroy(&current);
                    return false;
                }
                current.data[current.length++] = ch;
                current.data[current.length] = '\0';
            }
        }
    }

    if (saw_cr) {
        if (!line_reserve(&current, current.length + 2)) {
            fclose(file);
            line_destroy(&current);
            return false;
        }
        current.data[current.length++] = '\r';
        current.data[current.length] = '\0';
    }

    document_append_line(doc, current.data == NULL ? "" : current.data, current.length);
    line_destroy(&current);
    fclose(file);

    if (!document_set_path(doc, path)) return false;
    doc->dirty = false;
    return true;
}

bool document_save(Document *doc) {
    if (doc->path == NULL) return false;
    return document_save_as(doc, doc->path);
}

bool document_save_as(Document *doc, const char *path) {
    char temp_path[4096];
    if (!make_temp_path(path, temp_path, sizeof(temp_path))) return false;

#ifndef _WIN32
    struct stat existing_stat;
    bool preserve_mode = stat(path, &existing_stat) == 0;
#endif

    FILE *file = fopen(temp_path, "wb");
    if (file == NULL) return false;

    const char *separator = document_line_separator(doc);
    size_t separator_length = strlen(separator);
    for (size_t i = 0; i < doc->line_count; i++) {
        if (fwrite(doc->lines[i].data == NULL ? "" : doc->lines[i].data, 1, doc->lines[i].length, file) != doc->lines[i].length) {
            fclose(file);
            remove(temp_path);
            return false;
        }
        if (i + 1 < doc->line_count && fwrite(separator, 1, separator_length, file) != separator_length) {
            fclose(file);
            remove(temp_path);
            return false;
        }
    }

    if (fflush(file) != 0) {
        fclose(file);
        remove(temp_path);
        return false;
    }
    if (fclose(file) != 0) {
        remove(temp_path);
        return false;
    }
#ifndef _WIN32
    if (preserve_mode && chmod(temp_path, existing_stat.st_mode & 0777) != 0) {
        remove(temp_path);
        return false;
    }
#endif
    if (!replace_file_with_temp(temp_path, path)) {
        remove(temp_path);
        return false;
    }
    if (!document_set_path(doc, path)) return false;
    doc->dirty = false;
    return true;
}

void document_insert_char(Document *doc, size_t row, size_t col, char ch) {
    if (row >= doc->line_count) return;
    TextLine *line = &doc->lines[row];
    if (col > line->length) col = line->length;
    if (!line_reserve(line, line->length + 2)) return;
    memmove(line->data + col + 1, line->data + col, line->length - col + 1);
    line->data[col] = ch;
    line->length++;
    doc->dirty = true;
}

void document_delete_char_before(Document *doc, size_t row, size_t col, size_t *out_row, size_t *out_col) {
    if (out_row != NULL) *out_row = row;
    if (out_col != NULL) *out_col = col;
    if (row >= doc->line_count) return;
    if (col == 0) {
        if (row == 0) return;
        TextLine *previous = &doc->lines[row - 1];
        TextLine *current = &doc->lines[row];
        size_t previous_length = previous->length;
        if (!line_insert_bytes(previous, previous->length, current->data == NULL ? "" : current->data, current->length)) return;
        document_remove_line(doc, row);
        doc->dirty = true;
        if (out_row != NULL) *out_row = row - 1;
        if (out_col != NULL) *out_col = previous_length;
        return;
    }
    TextLine *line = &doc->lines[row];
    if (col > line->length) col = line->length;
    memmove(line->data + col - 1, line->data + col, line->length - col + 1);
    line->length--;
    doc->dirty = true;
    if (out_col != NULL) *out_col = col - 1;
}

void document_delete_char_at(Document *doc, size_t row, size_t col) {
    if (row >= doc->line_count) return;
    TextLine *line = &doc->lines[row];
    if (col < line->length) {
        memmove(line->data + col, line->data + col + 1, line->length - col);
        line->length--;
        doc->dirty = true;
        return;
    }
    if (row + 1 >= doc->line_count) return;
    TextLine *next = &doc->lines[row + 1];
    if (!line_insert_bytes(line, line->length, next->data == NULL ? "" : next->data, next->length)) return;
    document_remove_line(doc, row + 1);
    doc->dirty = true;
}

void document_insert_newline(Document *doc, size_t row, size_t col, size_t *out_row, size_t *out_col) {
    if (out_row != NULL) *out_row = row;
    if (out_col != NULL) *out_col = col;
    if (row >= doc->line_count) return;
    TextLine *line = &doc->lines[row];
    if (col > line->length) col = line->length;
    size_t tail_length = line->length - col;
    if (!document_insert_line(doc, row + 1, line->data + col, tail_length)) return;
    line = &doc->lines[row];
    line->data[col] = '\0';
    line->length = col;
    doc->dirty = true;
    if (out_row != NULL) *out_row = row + 1;
    if (out_col != NULL) *out_col = 0;
}
