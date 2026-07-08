#include "core/document.h"
#include "renderer/screen_buffer.h"

#include <assert.h>

int main(void) {
    Document doc;
    document_init(&doc);
    assert(doc.line_count == 1);
    document_insert_char(&doc, 0, 0, 'A');
    assert(doc.lines[0].length == 1);
    document_delete_char_before(&doc, 0, 1);
    assert(doc.lines[0].length == 0);
    document_destroy(&doc);

    ScreenBuffer buffer;
    screen_buffer_init(&buffer);
    assert(screen_buffer_resize(&buffer, 10, 20) == 1);
    screen_buffer_destroy(&buffer);

    return 0;
}
