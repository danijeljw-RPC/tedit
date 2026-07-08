#ifndef TEDIT_SCREEN_BUFFER_H
#define TEDIT_SCREEN_BUFFER_H

#include <stddef.h>

typedef struct {
    char ch;
    int style;
} ScreenCell;

typedef struct {
    int rows;
    int cols;
    ScreenCell *cells;
} ScreenBuffer;

void screen_buffer_init(ScreenBuffer *buffer);
void screen_buffer_destroy(ScreenBuffer *buffer);
int screen_buffer_resize(ScreenBuffer *buffer, int rows, int cols);

#endif
