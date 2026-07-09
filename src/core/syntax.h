#ifndef TEDIT_SYNTAX_H
#define TEDIT_SYNTAX_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SYNTAX_TOKEN_NORMAL = 0,
    SYNTAX_TOKEN_KEYWORD,
    SYNTAX_TOKEN_TYPE,
    SYNTAX_TOKEN_STRING,
    SYNTAX_TOKEN_CHARACTER,
    SYNTAX_TOKEN_COMMENT,
    SYNTAX_TOKEN_NUMBER,
    SYNTAX_TOKEN_FUNCTION,
    SYNTAX_TOKEN_PREPROCESSOR,
    SYNTAX_TOKEN_HEADING,
    SYNTAX_TOKEN_LINK,
    SYNTAX_TOKEN_ERROR,
    SYNTAX_TOKEN_TODO
} SyntaxTokenType;

typedef struct {
    size_t start;
    size_t length;
    SyntaxTokenType type;
} SyntaxToken;

typedef struct {
    SyntaxToken *tokens;
    size_t count;
    size_t capacity;
} SyntaxTokenLine;

typedef struct {
    char *pattern;
    SyntaxTokenType type;
    bool case_insensitive;
} SyntaxRule;

typedef struct {
    char *name;
    char **filename_patterns;
    size_t filename_pattern_count;
    size_t filename_pattern_capacity;
    SyntaxRule *rules;
    size_t rule_count;
    size_t rule_capacity;
    bool builtin_c;
} SyntaxDefinition;

void syntax_token_line_init(SyntaxTokenLine *line);
void syntax_token_line_destroy(SyntaxTokenLine *line);
void syntax_token_line_clear(SyntaxTokenLine *line);
bool syntax_token_line_add(SyntaxTokenLine *line, size_t start, size_t length, SyntaxTokenType type);
SyntaxTokenType syntax_token_line_type_at(const SyntaxTokenLine *line, size_t col);

void syntax_definition_init(SyntaxDefinition *definition);
void syntax_definition_destroy(SyntaxDefinition *definition);
void syntax_definition_init_builtin_c(SyntaxDefinition *definition);
bool syntax_definition_load_nano_text(SyntaxDefinition *definition, const char *text);
bool syntax_definition_load_nano_file(SyntaxDefinition *definition, const char *path);
bool syntax_definition_matches_filename(const SyntaxDefinition *definition, const char *filename);
bool syntax_highlight_line(const SyntaxDefinition *definition, const char *data, size_t length, SyntaxTokenLine *out);

#endif
