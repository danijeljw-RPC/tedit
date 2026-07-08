#include "platform.h"

#ifdef TEDIT_PLATFORM_WINDOWS

#include <stdio.h>

bool platform_enter_raw_mode(Platform *platform) {
    platform->raw_mode_enabled = 1;
    return true;
}

void platform_leave_raw_mode(Platform *platform) {
    platform->raw_mode_enabled = 0;
}

int platform_read_key(Platform *platform) {
    (void)platform;
    return getchar();
}

int platform_key_from_escape_sequence(const char *sequence) {
    (void)sequence;
    return '\x1b';
}

void platform_write(Platform *platform, const char *data, int length) {
    (void)platform;
    fwrite(data, 1, (size_t)length, stdout);
}

bool platform_get_terminal_size(Platform *platform, int *rows, int *cols) {
    (void)platform;
    *rows = 24;
    *cols = 80;
    return true;
}

#endif
