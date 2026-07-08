# Per-Line Horizontal Scroll

## Problem

Traditional terminal editors often use one horizontal offset for the whole visible document. When the cursor moves across a long line, every visible line shifts horizontally.

This is noisy when only one line is long.

## Required behaviour

- Only the active line uses horizontal scrolling.
- Inactive visible lines render from column 0.
- The active line scrolls just enough to keep the cursor visible.
- Moving to another line resets the previous line's horizontal viewport.
- Returning to a long line starts from column 0 and scrolls again only if needed to keep the cursor visible.

## MVP model

Use one value initially:

```text
active_line_left_col
```

This means the currently active line has a horizontal left column. Other lines render from zero.

## Later model

Optional future enhancement:

```text
per_line_viewport_cache[line_id] = left_col
```

This would allow each long line to remember its own scroll position while the user moves around, but it is not the current product direction.

## Example

Terminal width: 20

```text
short line
this is a very very long active line
another short line
```

When cursor moves far right on line 2:

```text
short line
very long active li
another short line
```

Line 1 and line 3 do not shift.
