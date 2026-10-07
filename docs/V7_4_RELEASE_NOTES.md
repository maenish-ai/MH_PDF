# MaenPDF 7.4.1 — Zero-Freeze UX & Editing

- **Compile stabilization:** corrected the Qt `QVariant::toInt(bool*)` misuse in inserted-text move/resize and added a CI/static regression guard.
- Debounced visual zoom: rapid zoom updates scale instantly while expensive PDF re-rendering is committed only after input settles.
- Editing a page no longer invalidates image rendering for every visible neighboring page.
- Smaller virtualized page prefetch windows on low-memory and balanced systems.
- Inserted text can be selected, dragged, resized, edited, deleted, undone/redone, and partially struck through.
- Local form fields are functional: text fields, checkboxes, radio buttons, dropdowns, and fill mode. They are flattened on save for compatibility; native AcroForm authoring remains a future backend capability.
- Undo/Redo controls are visually prominent and every new overlay/form edit is recorded in QUndoStack.
- PDF Focus Mode hides application chrome while keeping the desktop window itself normal.
- Dark mode keeps PDF paper and page thumbnails white.
- Artistic lightweight UI polish uses gradients, accents, and clear active states without blur-heavy effects.
- Existing 7.3 performance, clean-upgrade, Android release-signing, security, bilingual, and no-cloud guarantees are retained.
