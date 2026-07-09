#ifndef TEDIT_ANSI_RENDERER_H
#define TEDIT_ANSI_RENDERER_H

#include "core/editor.h"
#include "platform/platform.h"

void ansi_render_editor(Platform *platform, const Editor *editor);
void ansi_clear_screen(Platform *platform);
int ansi_text_display_width(const char *text);

#endif
