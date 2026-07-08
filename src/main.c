#include "core/editor.h"
#include "platform/platform.h"

int main(int argc, char **argv) {
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
