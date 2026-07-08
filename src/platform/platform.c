#include "platform.h"

bool platform_init(Platform *platform) {
    platform->raw_mode_enabled = 0;
#ifdef TEDIT_PLATFORM_POSIX
    platform->original_termios = 0;
#endif
#ifdef TEDIT_PLATFORM_WINDOWS
    platform->input_handle = 0;
    platform->output_handle = 0;
    platform->original_input_mode = 0;
    platform->original_output_mode = 0;
#endif
    return true;
}

void platform_shutdown(Platform *platform) {
    platform_leave_raw_mode(platform);
}
