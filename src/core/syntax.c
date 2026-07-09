#include "syntax.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_string_syntax(const char *value) {
    if (value == NULL) return NULL;
    size_t length = strlen(value);
    char *copy = malloc(length + 1);
    if (copy == NULL) return NULL;
    memcpy(copy, value, length + 1);
    return copy;
}

static char ascii_lower_char(char ch) {
    return (char)tolower((unsigned char)ch);
}

void syntax_token_line_init(SyntaxTokenLine *line) {
    line->tokens = NULL;
    line->count = 0;
    line->capacity = 0;
}

void syntax_token_line_destroy(SyntaxTokenLine *line) {
    free(line->tokens);
    line->tokens = NULL;
    line->count = 0;
    line->capacity = 0;
}

void syntax_token_line_clear(SyntaxTokenLine *line) {
    line->count = 0;
}

static bool syntax_token_line_reserve(SyntaxTokenLine *line, size_t capacity) {
    if (capacity <= line->capacity) return true;
    SyntaxToken *next = realloc(line->tokens, capacity * sizeof(SyntaxToken));
    if (next == NULL) return false;
    line->tokens = next;
    line->capacity = capacity;
    return true;
}

bool syntax_token_line_add(SyntaxTokenLine *line, size_t start, size_t length, SyntaxTokenType type) {
    if (length == 0 || type == SYNTAX_TOKEN_NORMAL) return true;
    if (line->count == line->capacity) {
        size_t next_capacity = line->capacity == 0 ? 16 : line->capacity * 2;
        if (!syntax_token_line_reserve(line, next_capacity)) return false;
    }
    line->tokens[line->count++] = (SyntaxToken){start, length, type};
    return true;
}

SyntaxTokenType syntax_token_line_type_at(const SyntaxTokenLine *line, size_t col) {
    for (size_t i = line->count; i > 0; i--) {
        const SyntaxToken *token = &line->tokens[i - 1];
        if (col >= token->start && col < token->start + token->length) return token->type;
    }
    return SYNTAX_TOKEN_NORMAL;
}

void syntax_definition_init(SyntaxDefinition *definition) {
    definition->name = NULL;
    definition->filename_patterns = NULL;
    definition->filename_pattern_count = 0;
    definition->filename_pattern_capacity = 0;
    definition->rules = NULL;
    definition->rule_count = 0;
    definition->rule_capacity = 0;
    definition->builtin_c = false;
}

void syntax_definition_destroy(SyntaxDefinition *definition) {
    free(definition->name);
    for (size_t i = 0; i < definition->filename_pattern_count; i++) free(definition->filename_patterns[i]);
    free(definition->filename_patterns);
    for (size_t i = 0; i < definition->rule_count; i++) free(definition->rules[i].pattern);
    free(definition->rules);
    syntax_definition_init(definition);
}

static bool syntax_definition_set_name(SyntaxDefinition *definition, const char *name) {
    char *copy = copy_string_syntax(name);
    if (copy == NULL) return false;
    free(definition->name);
    definition->name = copy;
    return true;
}

static bool syntax_definition_add_filename_pattern(SyntaxDefinition *definition, const char *pattern) {
    if (definition->filename_pattern_count == definition->filename_pattern_capacity) {
        size_t next_capacity = definition->filename_pattern_capacity == 0 ? 4 : definition->filename_pattern_capacity * 2;
        char **next = realloc(definition->filename_patterns, next_capacity * sizeof(char *));
        if (next == NULL) return false;
        definition->filename_patterns = next;
        definition->filename_pattern_capacity = next_capacity;
    }
    definition->filename_patterns[definition->filename_pattern_count] = copy_string_syntax(pattern);
    if (definition->filename_patterns[definition->filename_pattern_count] == NULL) return false;
    definition->filename_pattern_count++;
    return true;
}

static bool syntax_definition_add_rule(SyntaxDefinition *definition, const char *pattern, SyntaxTokenType type, bool case_insensitive) {
    if (definition->rule_count == definition->rule_capacity) {
        size_t next_capacity = definition->rule_capacity == 0 ? 8 : definition->rule_capacity * 2;
        SyntaxRule *next = realloc(definition->rules, next_capacity * sizeof(SyntaxRule));
        if (next == NULL) return false;
        definition->rules = next;
        definition->rule_capacity = next_capacity;
    }
    SyntaxRule *rule = &definition->rules[definition->rule_count++];
    rule->pattern = copy_string_syntax(pattern);
    rule->type = type;
    rule->case_insensitive = case_insensitive;
    return rule->pattern != NULL;
}

void syntax_definition_init_builtin_c(SyntaxDefinition *definition) {
    syntax_definition_destroy(definition);
    syntax_definition_init(definition);
    definition->builtin_c = true;
    syntax_definition_set_name(definition, "c");
    syntax_definition_add_filename_pattern(definition, "\\.c$");
    syntax_definition_add_filename_pattern(definition, "\\.h$");
}

static bool is_word_byte(char ch) {
    return isalnum((unsigned char)ch) || ch == '_';
}

static bool word_equals(const char *data, size_t start, size_t length, const char *word) {
    size_t word_length = strlen(word);
    return length == word_length && memcmp(data + start, word, word_length) == 0;
}

static SyntaxTokenType c_word_type(const char *data, size_t start, size_t length) {
    static const char *keywords[] = {
        "if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue",
        "return", "struct", "enum", "typedef", "static", "const", "sizeof", "void", "int",
        "char", "long", "short", "float", "double", "unsigned", "signed", "bool"
    };
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (word_equals(data, start, length, keywords[i])) return SYNTAX_TOKEN_KEYWORD;
    }
    return SYNTAX_TOKEN_NORMAL;
}

static bool highlight_builtin_c(const char *data, size_t length, SyntaxTokenLine *out) {
    size_t first = 0;
    while (first < length && (data[first] == ' ' || data[first] == '\t')) first++;
    if (first < length && data[first] == '#') {
        return syntax_token_line_add(out, first, length - first, SYNTAX_TOKEN_PREPROCESSOR);
    }

    for (size_t i = 0; i < length;) {
        if (i + 1 < length && data[i] == '/' && data[i + 1] == '/') {
            return syntax_token_line_add(out, i, length - i, SYNTAX_TOKEN_COMMENT);
        }
        if (data[i] == '"') {
            size_t start = i++;
            while (i < length) {
                if (data[i] == '\\' && i + 1 < length) {
                    i += 2;
                    continue;
                }
                if (data[i++] == '"') break;
            }
            if (!syntax_token_line_add(out, start, i - start, SYNTAX_TOKEN_STRING)) return false;
            continue;
        }
        if (data[i] == '\'') {
            size_t start = i++;
            while (i < length) {
                if (data[i] == '\\' && i + 1 < length) {
                    i += 2;
                    continue;
                }
                if (data[i++] == '\'') break;
            }
            if (!syntax_token_line_add(out, start, i - start, SYNTAX_TOKEN_CHARACTER)) return false;
            continue;
        }
        if (isdigit((unsigned char)data[i])) {
            size_t start = i++;
            while (i < length && (isalnum((unsigned char)data[i]) || data[i] == '.')) i++;
            if (!syntax_token_line_add(out, start, i - start, SYNTAX_TOKEN_NUMBER)) return false;
            continue;
        }
        if (is_word_byte(data[i])) {
            size_t start = i++;
            while (i < length && is_word_byte(data[i])) i++;
            SyntaxTokenType type = c_word_type(data, start, i - start);
            if (!syntax_token_line_add(out, start, i - start, type)) return false;
            continue;
        }
        i++;
    }
    return true;
}

static bool pattern_matches_at(const char *data, size_t length, size_t start, const char *pattern, bool case_insensitive) {
    size_t pattern_length = strlen(pattern);
    if (pattern_length == 0 || start + pattern_length > length) return false;
    for (size_t i = 0; i < pattern_length; i++) {
        char left = data[start + i];
        char right = pattern[i];
        if (case_insensitive) {
            left = ascii_lower_char(left);
            right = ascii_lower_char(right);
        }
        if (left != right) return false;
    }
    if (pattern_length > 0 && is_word_byte(pattern[0])) {
        if (start > 0 && is_word_byte(data[start - 1])) return false;
        if (start + pattern_length < length && is_word_byte(data[start + pattern_length])) return false;
    }
    return true;
}

static bool apply_rules(const SyntaxDefinition *definition, const char *data, size_t length, SyntaxTokenLine *out) {
    for (size_t r = 0; r < definition->rule_count; r++) {
        const SyntaxRule *rule = &definition->rules[r];
        size_t pattern_length = strlen(rule->pattern);
        for (size_t i = 0; i + pattern_length <= length; i++) {
            if (pattern_matches_at(data, length, i, rule->pattern, rule->case_insensitive)) {
                if (!syntax_token_line_add(out, i, pattern_length, rule->type)) return false;
                i += pattern_length == 0 ? 0 : pattern_length - 1;
            }
        }
    }
    return true;
}

bool syntax_highlight_line(const SyntaxDefinition *definition, const char *data, size_t length, SyntaxTokenLine *out) {
    syntax_token_line_clear(out);
    if (definition == NULL || data == NULL) return true;
    if (definition->builtin_c && !highlight_builtin_c(data, length, out)) return false;
    return apply_rules(definition, data, length, out);
}

static SyntaxTokenType token_type_from_color(const char *color) {
    if (strstr(color, "red") != NULL) return SYNTAX_TOKEN_TODO;
    if (strstr(color, "green") != NULL) return SYNTAX_TOKEN_KEYWORD;
    if (strstr(color, "blue") != NULL) return SYNTAX_TOKEN_TYPE;
    if (strstr(color, "yellow") != NULL) return SYNTAX_TOKEN_STRING;
    if (strstr(color, "magenta") != NULL) return SYNTAX_TOKEN_PREPROCESSOR;
    if (strstr(color, "cyan") != NULL) return SYNTAX_TOKEN_COMMENT;
    return SYNTAX_TOKEN_KEYWORD;
}

static const char *skip_spaces(const char *p) {
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

static char *parse_quoted(const char **cursor) {
    const char *p = strchr(*cursor, '"');
    if (p == NULL) return NULL;
    p++;
    char buffer[512];
    size_t out = 0;
    while (*p != '\0' && *p != '"' && out + 1 < sizeof(buffer)) {
        if (*p == '\\' && p[1] != '\0' && p[1] != '"') {
            buffer[out++] = *p++;
            buffer[out++] = *p++;
            continue;
        }
        if (*p == '\\' && p[1] == '"') p++;
        buffer[out++] = *p++;
    }
    if (*p != '"') return NULL;
    buffer[out] = '\0';
    *cursor = p + 1;
    return copy_string_syntax(buffer);
}

static bool parse_syntax_line(SyntaxDefinition *definition, const char *line) {
    const char *cursor = line;
    char *name = parse_quoted(&cursor);
    if (name == NULL) return false;
    bool ok = syntax_definition_set_name(definition, name);
    free(name);
    while (ok) {
        char *pattern = parse_quoted(&cursor);
        if (pattern == NULL) break;
        ok = syntax_definition_add_filename_pattern(definition, pattern);
        free(pattern);
    }
    return ok;
}

static bool parse_color_line(SyntaxDefinition *definition, const char *line, bool case_insensitive) {
    const char *cursor = skip_spaces(line);
    char color[64];
    size_t i = 0;
    while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t' && i + 1 < sizeof(color)) {
        color[i++] = *cursor++;
    }
    color[i] = '\0';
    SyntaxTokenType type = token_type_from_color(color);
    bool ok = true;
    while (ok) {
        char *pattern = parse_quoted(&cursor);
        if (pattern == NULL) break;
        ok = syntax_definition_add_rule(definition, pattern, type, case_insensitive);
        free(pattern);
    }
    return ok;
}

bool syntax_definition_load_nano_text(SyntaxDefinition *definition, const char *text) {
    syntax_definition_destroy(definition);
    syntax_definition_init(definition);
    const char *line_start = text;
    while (line_start != NULL && *line_start != '\0') {
        const char *line_end = strchr(line_start, '\n');
        size_t length = line_end == NULL ? strlen(line_start) : (size_t)(line_end - line_start);
        char line[1024];
        if (length >= sizeof(line)) length = sizeof(line) - 1;
        memcpy(line, line_start, length);
        line[length] = '\0';

        const char *trimmed = skip_spaces(line);
        if (strncmp(trimmed, "syntax", 6) == 0 && isspace((unsigned char)trimmed[6])) {
            if (!parse_syntax_line(definition, trimmed + 6)) return false;
        } else if (strncmp(trimmed, "color", 5) == 0 && isspace((unsigned char)trimmed[5])) {
            if (!parse_color_line(definition, trimmed + 5, false)) return false;
        } else if (strncmp(trimmed, "icolor", 6) == 0 && isspace((unsigned char)trimmed[6])) {
            if (!parse_color_line(definition, trimmed + 6, true)) return false;
        }
        if (line_end == NULL) break;
        line_start = line_end + 1;
    }
    return true;
}

bool syntax_definition_load_nano_file(SyntaxDefinition *definition, const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) return false;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    long size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }
    char *buffer = malloc((size_t)size + 1);
    if (buffer == NULL) {
        fclose(file);
        return false;
    }
    size_t read_count = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    buffer[read_count] = '\0';
    bool ok = syntax_definition_load_nano_text(definition, buffer);
    free(buffer);
    return ok;
}

static bool pattern_suffix_match(const char *pattern, const char *filename) {
    char normalized[256];
    size_t out = 0;
    for (size_t i = 0; pattern[i] != '\0' && out + 1 < sizeof(normalized); i++) {
        if (pattern[i] == '\\') continue;
        if (pattern[i] == '$') break;
        normalized[out++] = pattern[i];
    }
    normalized[out] = '\0';
    const char *dot = strchr(normalized, '.');
    if (dot == NULL) return strstr(filename, normalized) != NULL;
    size_t suffix_length = strlen(dot);
    size_t filename_length = strlen(filename);
    return filename_length >= suffix_length &&
           strcmp(filename + filename_length - suffix_length, dot) == 0;
}

bool syntax_definition_matches_filename(const SyntaxDefinition *definition, const char *filename) {
    for (size_t i = 0; i < definition->filename_pattern_count; i++) {
        if (pattern_suffix_match(definition->filename_patterns[i], filename)) return true;
    }
    return false;
}
