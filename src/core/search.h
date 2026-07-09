#ifndef TEDIT_SEARCH_H
#define TEDIT_SEARCH_H

#include "document.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *query;
    size_t query_length;
    DocumentRange current;
    bool has_current;
    size_t match_count;
} SearchState;

void search_state_init(SearchState *search);
void search_state_destroy(SearchState *search);
bool search_set_query(SearchState *search, const char *query);
void search_clear(SearchState *search);
bool search_find_next(const Document *doc, SearchState *search, DocumentPosition from);
bool search_find_previous(const Document *doc, SearchState *search, DocumentPosition from);
size_t search_count_matches(const Document *doc, const SearchState *search);
bool search_range_contains(const SearchState *search, size_t row, size_t col);

#endif
