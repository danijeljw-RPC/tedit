#include "screen_buffer.h"
#include <stdlib.h>

void screen_buffer_init(ScreenBuffer *buffer) {
    buffer->rows = 0;
    buffer->cols = 0;
    buffer->cells = NULL;
}

void screen_buffer_destroy(ScreenBuffer *buffer) {
    free(buffer->cells);
    buffer->cells = NULL;
    buffer->rows = 0;
    buffer->cols = 0;
}

int screen_buffer_resize(ScreenBuffer *buffer, int rows, int cols) {
    if (rows <= 0 || cols <= 0) return 0;
    ScreenCell *cells = realloc(buffer->cells, (size_t)rows * (size_t)cols * sizeof(ScreenCell));
    if (cells == NULL) return 0;
    buffer->cells = cells;
    buffer->rows = rows;
    buffer->cols = cols;
    return 1;
}
