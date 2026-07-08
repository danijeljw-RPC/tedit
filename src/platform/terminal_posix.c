#include "platform.h"

#ifndef TEDIT_PLATFORM_WINDOWS

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

bool platform_enter_raw_mode(Platform *platform) {
    if (platform->raw_mode_enabled) return true;

    struct termios *original = malloc(sizeof(struct termios));
    if (original == NULL) return false;
    if (tcgetattr(STDIN_FILENO, original) == -1) {
        free(original);
        return false;
    }

    struct termios raw = *original;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        free(original);
        return false;
    }

    platform->original_termios = original;
    platform->raw_mode_enabled = 1;
    return true;
}

void platform_leave_raw_mode(Platform *platform) {
    if (!platform->raw_mode_enabled) return;
    struct termios *original = (struct termios *)platform->original_termios;
    if (original != NULL) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, original);
        free(original);
    }
    platform->original_termios = NULL;
    platform->raw_mode_enabled = 0;
}

int platform_read_key(Platform *platform) {
    (void)platform;
    char c;
    while (read(STDIN_FILENO, &c, 1) != 1) {}

    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return TEDIT_KEY_ARROW_UP;
                case 'B': return TEDIT_KEY_ARROW_DOWN;
                case 'C': return TEDIT_KEY_ARROW_RIGHT;
                case 'D': return TEDIT_KEY_ARROW_LEFT;
            }
        }
        return '\x1b';
    }

    return c;
}

void platform_write(Platform *platform, const char *data, int length) {
    (void)platform;
    write(STDOUT_FILENO, data, (size_t)length);
}

bool platform_get_terminal_size(Platform *platform, int *rows, int *cols) {
    (void)platform;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        *rows = 24;
        *cols = 80;
        return false;
    }
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return true;
}

#endif
