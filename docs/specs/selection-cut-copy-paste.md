# Selection, Cut, Copy, Paste

## Required behaviour

- Shift+arrows extend selection.
- Typing while selection exists replaces the selection.
- Backspace/Delete while selection exists deletes the selection.
- Cut removes selected text and stores it in the internal clipboard.
- Copy stores selected text without deleting.
- Paste inserts the internal clipboard at the cursor.

## Line operations

- Cut line should work even without an active selection.
- Copy line should work even without an active selection.

## Later

- OS clipboard integration
- rectangular/block selection
- mouse selection
