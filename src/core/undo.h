#ifndef TEDIT_UNDO_H
#define TEDIT_UNDO_H

#include "document.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    DocumentPosition start;
    DocumentPosition end_before;
    DocumentPosition cursor_before;
    DocumentPosition cursor_after;
    char *deleted;
    size_t deleted_length;
    char *inserted;
    size_t inserted_length;
} UndoRecord;

typedef struct {
    UndoRecord *undo;
    size_t undo_count;
    size_t undo_capacity;
    UndoRecord *redo;
    size_t redo_count;
    size_t redo_capacity;
} UndoStack;

void undo_stack_init(UndoStack *stack);
void undo_stack_destroy(UndoStack *stack);
bool undo_stack_push_edit(UndoStack *stack, UndoRecord record, bool merge_insert);
bool undo_stack_pop_undo(UndoStack *stack, UndoRecord *record);
bool undo_stack_pop_redo(UndoStack *stack, UndoRecord *record);
bool undo_stack_push_redo(UndoStack *stack, UndoRecord record);
bool undo_stack_push_undo_record(UndoStack *stack, UndoRecord record);
void undo_stack_clear_redo(UndoStack *stack);
void undo_record_destroy(UndoRecord *record);

#endif
