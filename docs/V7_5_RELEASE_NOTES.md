# MaenPDF 7.5.0 — Highlight, Unicode & Full Screen

- Added a multi-color highlight palette shared by text/area highlights and the freehand Highlight Pen.
- Added freehand Highlight Pen with adjustable opacity and width while reusing the lightweight touched-patch stroke engine.
- Undo/Redo is no longer capped at 5/10/20 operations; QUndoStack uses unlimited history while common edits store compact vector/touched-patch state.
- Renamed PDF Focus Mode to **PDF Full Screen Mode** and changed it to true operating-system fullscreen with all MaenPDF chrome hidden; Esc/Ctrl+L exits.
- Added compatibility normalization for editable Unicode text, including Arabic Presentation Forms, while preserving Arabic/English editing, copy, paste and movement through QString.
- Inserted text and local form values render with automatic RTL/LTR direction using QTextOption, supporting Arabic, English and mixed bidi content.
- Existing adaptive cache/render budgets remain in place for older 4 GB hardware.
