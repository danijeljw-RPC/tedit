#ifndef TEDIT_DOCUMENT_H
#define TEDIT_DOCUMENT_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} TextLine;

typedef struct {
    TextLine *lines;
    size_t line_count;
    size_t capacity;
    char *path;
    bool dirty;
} Document;

void document_init(Document *doc);
void document_destroy(Document *doc);
bool document_open(Document *doc, const char *path);
bool document_save(Document *doc);
bool document_save_as(Document *doc, const char *path);

void document_insert_char(Document *doc, size_t row, size_t col, char ch);
void document_delete_char_before(Document *doc, size_t row, size_t col);
void document_insert_newline(Document *doc, size_t row, size_t col);

#endif
