#include "undo.h"

#include <stdlib.h>
#include <string.h>

static char *copy_bytes(const char *data, size_t length) {
    char *copy = malloc(length + 1);
    if (copy == NULL) return NULL;
    if (length > 0) memcpy(copy, data, length);
    copy[length] = '\0';
    return copy;
}

static bool reserve_records(UndoRecord **records, size_t *capacity, size_t needed) {
    if (needed <= *capacity) return true;
    size_t next_capacity = *capacity == 0 ? 16 : *capacity * 2;
    while (next_capacity < needed) next_capacity *= 2;
    UndoRecord *next = realloc(*records, next_capacity * sizeof(UndoRecord));
    if (next == NULL) return false;
    *records = next;
    *capacity = next_capacity;
    return true;
}

static bool record_owns_valid_buffers(const UndoRecord *record) {
    return record->deleted != NULL && record->inserted != NULL;
}

static bool can_merge_insert(const UndoRecord *previous, const UndoRecord *next) {
    if (previous->deleted_length != 0 || next->deleted_length != 0) return false;
    if (previous->cursor_after.row != next->start.row || previous->cursor_after.col != next->start.col) return false;
    if (next->inserted_length != 1 || next->inserted[0] == '\n') return false;
    for (size_t i = 0; i < previous->inserted_length; i++) {
        if (previous->inserted[i] == '\n') return false;
    }
    return true;
}

void undo_stack_init(UndoStack *stack) {
    stack->undo = NULL;
    stack->undo_count = 0;
    stack->undo_capacity = 0;
    stack->redo = NULL;
    stack->redo_count = 0;
    stack->redo_capacity = 0;
}

void undo_record_destroy(UndoRecord *record) {
    free(record->deleted);
    free(record->inserted);
    record->deleted = NULL;
    record->inserted = NULL;
    record->deleted_length = 0;
    record->inserted_length = 0;
}

void undo_stack_clear_redo(UndoStack *stack) {
    for (size_t i = 0; i < stack->redo_count; i++) {
        undo_record_destroy(&stack->redo[i]);
    }
    stack->redo_count = 0;
}

void undo_stack_destroy(UndoStack *stack) {
    for (size_t i = 0; i < stack->undo_count; i++) {
        undo_record_destroy(&stack->undo[i]);
    }
    for (size_t i = 0; i < stack->redo_count; i++) {
        undo_record_destroy(&stack->redo[i]);
    }
    free(stack->undo);
    free(stack->redo);
    stack->undo = NULL;
    stack->redo = NULL;
    stack->undo_count = 0;
    stack->redo_count = 0;
    stack->undo_capacity = 0;
    stack->redo_capacity = 0;
}

bool undo_stack_push_edit(UndoStack *stack, UndoRecord record, bool merge_insert) {
    if (record.deleted == NULL) record.deleted = copy_bytes("", 0);
    if (record.inserted == NULL) record.inserted = copy_bytes("", 0);
    if (!record_owns_valid_buffers(&record)) {
        undo_record_destroy(&record);
        return false;
    }

    undo_stack_clear_redo(stack);

    if (merge_insert && stack->undo_count > 0) {
        UndoRecord *previous = &stack->undo[stack->undo_count - 1];
        if (can_merge_insert(previous, &record)) {
            char *next_inserted = realloc(previous->inserted, previous->inserted_length + record.inserted_length + 1);
            if (next_inserted == NULL) {
                undo_record_destroy(&record);
                return false;
            }
            previous->inserted = next_inserted;
            memcpy(previous->inserted + previous->inserted_length, record.inserted, record.inserted_length);
            previous->inserted_length += record.inserted_length;
            previous->inserted[previous->inserted_length] = '\0';
            previous->cursor_after = record.cursor_after;
            undo_record_destroy(&record);
            return true;
        }
    }

    if (!reserve_records(&stack->undo, &stack->undo_capacity, stack->undo_count + 1)) {
        undo_record_destroy(&record);
        return false;
    }
    stack->undo[stack->undo_count++] = record;
    return true;
}

bool undo_stack_pop_undo(UndoStack *stack, UndoRecord *record) {
    if (stack->undo_count == 0) return false;
    *record = stack->undo[--stack->undo_count];
    return true;
}

bool undo_stack_pop_redo(UndoStack *stack, UndoRecord *record) {
    if (stack->redo_count == 0) return false;
    *record = stack->redo[--stack->redo_count];
    return true;
}

bool undo_stack_push_redo(UndoStack *stack, UndoRecord record) {
    if (!reserve_records(&stack->redo, &stack->redo_capacity, stack->redo_count + 1)) {
        undo_record_destroy(&record);
        return false;
    }
    stack->redo[stack->redo_count++] = record;
    return true;
}

bool undo_stack_push_undo_record(UndoStack *stack, UndoRecord record) {
    if (!reserve_records(&stack->undo, &stack->undo_capacity, stack->undo_count + 1)) {
        undo_record_destroy(&record);
        return false;
    }
    stack->undo[stack->undo_count++] = record;
    return true;
}
