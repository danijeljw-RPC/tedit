#include "core/editor.h"
#include "core/app_info.h"
#include "platform/platform.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc > 1 && (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
        printf("%s\n%s\n", tedit_app_version_title(), tedit_app_copyright());
        return 0;
    }

    const char *path = argc > 1 ? argv[1] : NULL;

    Platform platform;
    if (!platform_init(&platform)) {
        return 1;
    }

    Editor editor;
    editor_init(&editor);

    if (path != NULL) {
        editor_open(&editor, path);
    }

    int result = editor_run(&editor, &platform);

    editor_destroy(&editor);
    platform_shutdown(&platform);
    return result;
}
