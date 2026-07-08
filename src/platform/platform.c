#include "platform.h"

bool platform_init(Platform *platform) {
    platform->raw_mode_enabled = 0;
#ifdef TEDIT_PLATFORM_POSIX
    platform->original_termios = 0;
#endif
    return true;
}

void platform_shutdown(Platform *platform) {
    platform_leave_raw_mode(platform);
}
