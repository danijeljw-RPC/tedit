#include "document.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void document_init(Document *doc) {
    doc->lines = NULL;
    doc->line_count = 0;
    doc->capacity = 0;
    doc->path = NULL;
    doc->dirty = false;
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
    FILE *file = fopen(path, "rb");
    if (file == NULL) return false;

    for (size_t i = 0; i < doc->line_count; i++) line_destroy(&doc->lines[i]);
    doc->line_count = 0;

    char buffer[4096];
    TextLine current;
    line_init(&current);

    size_t read_count;
    while ((read_count = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < read_count; i++) {
            char ch = buffer[i];
            if (ch == '\r') continue;
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

    document_append_line(doc, current.data == NULL ? "" : current.data, current.length);
    line_destroy(&current);
    fclose(file);

    free(doc->path);
    doc->path = copy_string(path);
    doc->dirty = false;
    return true;
}

bool document_save(Document *doc) {
    if (doc->path == NULL) return false;
    return document_save_as(doc, doc->path);
}

bool document_save_as(Document *doc, const char *path) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) return false;

    for (size_t i = 0; i < doc->line_count; i++) {
        fwrite(doc->lines[i].data == NULL ? "" : doc->lines[i].data, 1, doc->lines[i].length, file);
        if (i + 1 < doc->line_count) fputc('\n', file);
    }

    fclose(file);
    free(doc->path);
    doc->path = copy_string(path);
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

void document_delete_char_before(Document *doc, size_t row, size_t col) {
    if (row >= doc->line_count || col == 0) return;
    TextLine *line = &doc->lines[row];
    if (col > line->length) col = line->length;
    memmove(line->data + col - 1, line->data + col, line->length - col + 1);
    line->length--;
    doc->dirty = true;
}

void document_insert_newline(Document *doc, size_t row, size_t col) {
    (void)doc;
    (void)row;
    (void)col;
    /* MVP TODO: split current line and insert a new TextLine. */
}
