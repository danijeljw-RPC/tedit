#ifndef TEDIT_PLATFORM_H
#define TEDIT_PLATFORM_H

#include <stdbool.h>

#define TEDIT_KEY_CTRL_Q 17
#define TEDIT_KEY_CTRL_S 19
#define TEDIT_KEY_BACKSPACE 127
#define TEDIT_KEY_ARROW_LEFT 1000
#define TEDIT_KEY_ARROW_RIGHT 1001
#define TEDIT_KEY_ARROW_UP 1002
#define TEDIT_KEY_ARROW_DOWN 1003
#define TEDIT_KEY_DELETE 1004
#define TEDIT_KEY_HOME 1005
#define TEDIT_KEY_END 1006
#define TEDIT_KEY_PAGE_UP 1007
#define TEDIT_KEY_PAGE_DOWN 1008
#define TEDIT_KEY_CTRL_HOME 1009
#define TEDIT_KEY_CTRL_END 1010

typedef struct Platform {
    int raw_mode_enabled;
#ifdef TEDIT_PLATFORM_POSIX
    void *original_termios;
#endif
#ifdef TEDIT_PLATFORM_WINDOWS
    void *input_handle;
    void *output_handle;
    unsigned long original_input_mode;
    unsigned long original_output_mode;
#endif
} Platform;

bool platform_init(Platform *platform);
void platform_shutdown(Platform *platform);
bool platform_enter_raw_mode(Platform *platform);
void platform_leave_raw_mode(Platform *platform);
int platform_read_key(Platform *platform);
int platform_key_from_escape_sequence(const char *sequence);
void platform_write(Platform *platform, const char *data, int length);
bool platform_get_terminal_size(Platform *platform, int *rows, int *cols);

#endif
