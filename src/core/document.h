#ifndef TEDIT_DOCUMENT_H
#define TEDIT_DOCUMENT_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    DOCUMENT_LINE_ENDING_LF,
    DOCUMENT_LINE_ENDING_CRLF
} DocumentLineEnding;

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} TextLine;

typedef struct {
    size_t row;
    size_t col;
} DocumentPosition;

typedef struct {
    DocumentPosition start;
    DocumentPosition end;
} DocumentRange;

typedef struct {
    TextLine *lines;
    size_t line_count;
    size_t capacity;
    char *path;
    bool dirty;
    DocumentLineEnding line_ending;
} Document;

void document_init(Document *doc);
void document_destroy(Document *doc);
bool document_open(Document *doc, const char *path);
bool document_load_path(Document *doc, const char *path, bool *created_new);
bool document_save(Document *doc);
bool document_save_as(Document *doc, const char *path);

void document_insert_char(Document *doc, size_t row, size_t col, char ch);
void document_delete_char_before(Document *doc, size_t row, size_t col, size_t *out_row, size_t *out_col);
void document_delete_char_at(Document *doc, size_t row, size_t col);
void document_insert_newline(Document *doc, size_t row, size_t col, size_t *out_row, size_t *out_col);
int document_position_compare(DocumentPosition left, DocumentPosition right);
DocumentPosition document_clamp_position(const Document *doc, DocumentPosition position);
DocumentRange document_range_normalize(const Document *doc, DocumentRange range);
char *document_extract_range(const Document *doc, DocumentRange range);
char *document_extract_range_with_length(const Document *doc, DocumentRange range, size_t *out_length);
DocumentPosition document_delete_range(Document *doc, DocumentRange range);
DocumentPosition document_insert_bytes(Document *doc, DocumentPosition position, const char *data, size_t length);
DocumentPosition document_replace_range(Document *doc, DocumentRange range, const char *data, size_t length);

#endif
