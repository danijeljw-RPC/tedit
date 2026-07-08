#include "platform.h"

#ifdef TEDIT_PLATFORM_WINDOWS

#include <windows.h>

static bool control_pressed(const KEY_EVENT_RECORD *event) {
    DWORD state = event->dwControlKeyState;
    return (state & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
}

static int key_from_record(const KEY_EVENT_RECORD *event) {
    if (!event->bKeyDown) return 0;

    switch (event->wVirtualKeyCode) {
        case VK_LEFT: return TEDIT_KEY_ARROW_LEFT;
        case VK_RIGHT: return TEDIT_KEY_ARROW_RIGHT;
        case VK_UP: return TEDIT_KEY_ARROW_UP;
        case VK_DOWN: return TEDIT_KEY_ARROW_DOWN;
        case VK_HOME: return control_pressed(event) ? TEDIT_KEY_CTRL_HOME : TEDIT_KEY_HOME;
        case VK_END: return control_pressed(event) ? TEDIT_KEY_CTRL_END : TEDIT_KEY_END;
        case VK_PRIOR: return TEDIT_KEY_PAGE_UP;
        case VK_NEXT: return TEDIT_KEY_PAGE_DOWN;
        case VK_DELETE: return TEDIT_KEY_DELETE;
        case VK_BACK: return TEDIT_KEY_BACKSPACE;
        case VK_RETURN: return '\r';
        default:
            break;
    }

    WCHAR ch = event->uChar.UnicodeChar;
    if (ch == 0) return 0;
    if (ch == L'\t') return '\t';
    if (ch == 17) return TEDIT_KEY_CTRL_Q;
    if (ch == 19) return TEDIT_KEY_CTRL_S;
    if (ch >= 1 && ch <= 255) return (int)ch;
    return 0;
}

bool platform_enter_raw_mode(Platform *platform) {
    if (platform->raw_mode_enabled) return true;

    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (input == INVALID_HANDLE_VALUE || output == INVALID_HANDLE_VALUE) return false;

    DWORD input_mode = 0;
    DWORD output_mode = 0;
    if (!GetConsoleMode(input, &input_mode)) return false;
    if (!GetConsoleMode(output, &output_mode)) return false;

    DWORD raw_input = input_mode;
    raw_input &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    raw_input |= ENABLE_WINDOW_INPUT;

    DWORD raw_output = output_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    if (!SetConsoleMode(input, raw_input)) return false;
    if (!SetConsoleMode(output, raw_output)) {
        SetConsoleMode(input, input_mode);
        return false;
    }

    platform->input_handle = input;
    platform->output_handle = output;
    platform->original_input_mode = input_mode;
    platform->original_output_mode = output_mode;
    platform->raw_mode_enabled = 1;
    return true;
}

void platform_leave_raw_mode(Platform *platform) {
    if (!platform->raw_mode_enabled) return;
    HANDLE input = (HANDLE)platform->input_handle;
    HANDLE output = (HANDLE)platform->output_handle;
    if (input != NULL && input != INVALID_HANDLE_VALUE) {
        SetConsoleMode(input, (DWORD)platform->original_input_mode);
    }
    if (output != NULL && output != INVALID_HANDLE_VALUE) {
        SetConsoleMode(output, (DWORD)platform->original_output_mode);
    }
    platform->input_handle = NULL;
    platform->output_handle = NULL;
    platform->raw_mode_enabled = 0;
}

int platform_read_key(Platform *platform) {
    HANDLE input = (HANDLE)platform->input_handle;
    if (input == NULL || input == INVALID_HANDLE_VALUE) input = GetStdHandle(STD_INPUT_HANDLE);

    for (;;) {
        INPUT_RECORD record;
        DWORD read_count = 0;
        if (!ReadConsoleInputA(input, &record, 1, &read_count) || read_count == 0) {
            return 0;
        }
        if (record.EventType != KEY_EVENT) continue;

        int key = key_from_record(&record.Event.KeyEvent);
        if (key != 0) return key;
    }
}

int platform_key_from_escape_sequence(const char *sequence) {
    (void)sequence;
    return '\x1b';
}

void platform_write(Platform *platform, const char *data, int length) {
    HANDLE output = (HANDLE)platform->output_handle;
    if (output == NULL || output == INVALID_HANDLE_VALUE) output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written = 0;
    WriteFile(output, data, (DWORD)length, &written, NULL);
}

bool platform_get_terminal_size(Platform *platform, int *rows, int *cols) {
    HANDLE output = (HANDLE)platform->output_handle;
    if (output == NULL || output == INVALID_HANDLE_VALUE) output = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(output, &info)) {
        *rows = 24;
        *cols = 80;
        return false;
    }
    *cols = info.srWindow.Right - info.srWindow.Left + 1;
    *rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    return true;
}

#endif
