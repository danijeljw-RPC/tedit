#include "platform.h"

#ifndef TEDIT_PLATFORM_WINDOWS

#include <stdlib.h>
#include <string.h>
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

int platform_key_from_escape_sequence(const char *sequence) {
    if (strcmp(sequence, "q") == 0 || strcmp(sequence, "Q") == 0) return TEDIT_KEY_CTRL_Q;
    if (strcmp(sequence, "[A") == 0) return TEDIT_KEY_ARROW_UP;
    if (strcmp(sequence, "[B") == 0) return TEDIT_KEY_ARROW_DOWN;
    if (strcmp(sequence, "[C") == 0) return TEDIT_KEY_ARROW_RIGHT;
    if (strcmp(sequence, "[D") == 0) return TEDIT_KEY_ARROW_LEFT;
    if (strcmp(sequence, "[H") == 0 || strcmp(sequence, "[1~") == 0) return TEDIT_KEY_HOME;
    if (strcmp(sequence, "[F") == 0 || strcmp(sequence, "[4~") == 0) return TEDIT_KEY_END;
    if (strcmp(sequence, "[3~") == 0) return TEDIT_KEY_DELETE;
    if (strcmp(sequence, "[5~") == 0) return TEDIT_KEY_PAGE_UP;
    if (strcmp(sequence, "[6~") == 0) return TEDIT_KEY_PAGE_DOWN;
    if (strcmp(sequence, "[1;5H") == 0) return TEDIT_KEY_CTRL_HOME;
    if (strcmp(sequence, "[1;5F") == 0) return TEDIT_KEY_CTRL_END;
    return '\x1b';
}

int platform_read_key(Platform *platform) {
    (void)platform;
    char c;
    while (read(STDIN_FILENO, &c, 1) != 1) {}

    if (c == '\x1b') {
        char seq[8];
        size_t len = 0;
        while (len + 1 < sizeof(seq)) {
            char next;
            if (read(STDIN_FILENO, &next, 1) != 1) break;
            seq[len++] = next;
            seq[len] = '\0';
            if ((next >= 'A' && next <= 'Z') || (next >= 'a' && next <= 'z') || next == '~') {
                break;
            }
        }
        return len == 0 ? '\x1b' : platform_key_from_escape_sequence(seq);
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
