# MaenPDF 7.2.0 — Interaction & Performance Release

This release replaces the single-page workspace with a virtualized continuous document viewer and turns the primary pointer tools into working document interactions.

## Viewer and navigation
- Continuous vertical PDF scrolling with the mouse wheel / touchpad.
- Current page follows the viewport while scrolling; thumbnail clicks and Page Up/Page Down keep the viewer synchronized.
- Two-direction flicking remains available when a zoomed page is wider than the viewport.
- ListView delegate reuse and bounded cacheBuffer keep long documents light.
- Full-page renders and thumbnails request images asynchronously; Qt PDF access is serialized to avoid concurrent backend access.

## Pointer and editing tools
- Pointer drag selects real source PDF text using `QPdfDocument::getSelection()` and returns geometric text bounds.
- Selected text can be copied or converted to a real multi-rectangle highlight.
- Highlight drag follows selectable PDF text, with a rectangular fallback for scanned/image-only pages.
- Draw has a live canvas preview, configurable color and brush width, then commits one undoable ink operation.
- Crop is staged: drag the area, adjust corner handles, then Apply or Cancel.
- Redaction is staged: drag, review, then Apply. Saved MaenPDF output remains flattened so covered source content is not retained in the exported PDF stream.
- Text insertion continues to work by clicking the desired page position.

## Responsiveness
- Multi-page export, image export, compare, OCR orchestration and local provider waits periodically pump non-input events so Windows does not falsely report the app as hung during long local operations.
- Page rendering uses short-lived page snapshots for asynchronous requests.
- Closed tabs are retired briefly before deletion so in-flight image-provider work cannot dereference a freed document.

## CI coverage
The Windows workflow now runs a three-page PDF fixture through an interaction smoke mode that verifies text selection, highlight, drawing, redaction, crop, page navigation, undo state and QML startup before the installer is produced.

## Capability boundary
Structural rewriting of existing PDF text objects, certificate-backed digital signatures, AcroForm authoring and true object-tree redaction remain deliberately disabled until a verified structural writer/provider is integrated.
