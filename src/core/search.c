#include "search.h"

#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *value) {
    size_t length = value == NULL ? 0 : strlen(value);
    char *copy = malloc(length + 1);
    if (copy == NULL) return NULL;
    if (length > 0) memcpy(copy, value, length);
    copy[length] = '\0';
    return copy;
}

static bool line_matches_at(const TextLine *line, size_t col, const char *query, size_t query_length) {
    if (query_length == 0 || col + query_length > line->length) return false;
    return memcmp(line->data + col, query, query_length) == 0;
}

static bool find_in_line_forward(const TextLine *line, const char *query, size_t query_length, size_t start_col, size_t *match_col) {
    if (query_length == 0 || start_col > line->length) return false;
    for (size_t col = start_col; col + query_length <= line->length; col++) {
        if (line_matches_at(line, col, query, query_length)) {
            *match_col = col;
            return true;
        }
    }
    return false;
}

static bool find_in_line_backward(const TextLine *line, const char *query, size_t query_length, size_t start_col, size_t *match_col) {
    if (query_length == 0 || line->length < query_length) return false;
    size_t max_col = line->length - query_length;
    if (start_col < max_col) max_col = start_col;
    for (size_t col = max_col + 1; col > 0; col--) {
        size_t candidate = col - 1;
        if (line_matches_at(line, candidate, query, query_length)) {
            *match_col = candidate;
            return true;
        }
    }
    return false;
}

static void set_current_match(SearchState *search, size_t row, size_t col) {
    search->current.start.row = row;
    search->current.start.col = col;
    search->current.end.row = row;
    search->current.end.col = col + search->query_length;
    search->has_current = true;
}

void search_state_init(SearchState *search) {
    search->query = NULL;
    search->query_length = 0;
    search->current.start.row = 0;
    search->current.start.col = 0;
    search->current.end = search->current.start;
    search->has_current = false;
    search->match_count = 0;
}

void search_state_destroy(SearchState *search) {
    free(search->query);
    search->query = NULL;
    search->query_length = 0;
    search->has_current = false;
    search->match_count = 0;
}

bool search_set_query(SearchState *search, const char *query) {
    char *copy = copy_string(query == NULL ? "" : query);
    if (copy == NULL) return false;
    free(search->query);
    search->query = copy;
    search->query_length = strlen(copy);
    search->has_current = false;
    search->match_count = 0;
    return search->query_length > 0;
}

void search_clear(SearchState *search) {
    free(search->query);
    search_state_init(search);
}

size_t search_count_matches(const Document *doc, const SearchState *search) {
    if (search->query == NULL || search->query_length == 0) return 0;
    size_t count = 0;
    for (size_t row = 0; row < doc->line_count; row++) {
        size_t col = 0;
        while (find_in_line_forward(&doc->lines[row], search->query, search->query_length, col, &col)) {
            count++;
            col++;
        }
    }
    return count;
}

bool search_find_next(const Document *doc, SearchState *search, DocumentPosition from) {
    if (search->query == NULL || search->query_length == 0 || doc->line_count == 0) return false;
    from = document_clamp_position(doc, from);
    size_t start_row = from.row;
    size_t start_col = from.col;
    if (search->has_current) {
        start_row = search->current.start.row;
        start_col = search->current.start.col + 1;
        if (start_row >= doc->line_count) start_row = 0;
    }

    for (size_t row = start_row; row < doc->line_count; row++) {
        size_t col = row == start_row ? start_col : 0;
        size_t match_col = 0;
        if (find_in_line_forward(&doc->lines[row], search->query, search->query_length, col, &match_col)) {
            set_current_match(search, row, match_col);
            search->match_count = search_count_matches(doc, search);
            return true;
        }
    }
    for (size_t row = 0; row <= start_row && row < doc->line_count; row++) {
        size_t match_col = 0;
        if (find_in_line_forward(&doc->lines[row], search->query, search->query_length, 0, &match_col) &&
            (row != start_row || match_col < start_col)) {
            set_current_match(search, row, match_col);
            search->match_count = search_count_matches(doc, search);
            return true;
        }
    }
    search->has_current = false;
    search->match_count = 0;
    return false;
}

bool search_find_previous(const Document *doc, SearchState *search, DocumentPosition from) {
    if (search->query == NULL || search->query_length == 0 || doc->line_count == 0) return false;
    from = document_clamp_position(doc, from);
    size_t start_row = from.row;
    size_t start_col = from.col;
    if (search->has_current) {
        start_row = search->current.start.row;
        start_col = search->current.start.col == 0 ? 0 : search->current.start.col - 1;
    }

    for (size_t row = start_row + 1; row > 0; row--) {
        size_t current_row = row - 1;
        const TextLine *line = &doc->lines[current_row];
        if (current_row == start_row && start_col == 0 && search->has_current) continue;
        size_t col = current_row == start_row ? start_col : line->length;
        size_t match_col = 0;
        if (find_in_line_backward(line, search->query, search->query_length, col, &match_col)) {
            set_current_match(search, current_row, match_col);
            search->match_count = search_count_matches(doc, search);
            return true;
        }
    }
    for (size_t row = doc->line_count; row > start_row; row--) {
        size_t current_row = row - 1;
        const TextLine *line = &doc->lines[current_row];
        size_t match_col = 0;
        if (find_in_line_backward(line, search->query, search->query_length, line->length, &match_col) &&
            (current_row != start_row || match_col > start_col)) {
            set_current_match(search, current_row, match_col);
            search->match_count = search_count_matches(doc, search);
            return true;
        }
    }
    if (search->has_current && search_count_matches(doc, search) == 1) {
        set_current_match(search, search->current.start.row, search->current.start.col);
        search->match_count = 1;
        return true;
    }
    search->has_current = false;
    search->match_count = 0;
    return false;
}

bool search_range_contains(const SearchState *search, size_t row, size_t col) {
    if (!search->has_current) return false;
    return search->current.start.row == row &&
           search->current.end.row == row &&
           col >= search->current.start.col &&
           col < search->current.end.col;
}
