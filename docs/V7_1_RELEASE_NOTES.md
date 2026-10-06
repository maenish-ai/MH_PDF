# MaenPDF 7.1.0 — Professional Foundation

## Professional menu surface
MaenPDF now exposes a full desktop-class menu structure: File, Edit, View, Document, Pages, Comment, Forms, Protect, Convert, Tools, Window, Language and Help. Actions are enabled only when the current engine can perform them safely. Structural form authoring and certificate-backed digital signatures remain visible only as disabled future capabilities rather than misleading working commands.

## Native desktop printing
Windows desktop builds now include Qt Print Support and provide system Print, Print Current Page and Print Preview commands. The native print dialog provides the operating-system printer choices and page range controls. Printing renders through the same local page engine used by MaenPDF; no document is uploaded.

## Clean upgrades
The Windows installer cleans the application directory before every update so obsolete Qt plugins or runtime DLLs cannot remain mixed with a new build. A one-time settings-generation migration clears pre-7.1 application settings/cache while never touching user PDF documents. Future 7.1+ updates preserve compatible preferences unless the user selects the optional reset task.

## Settings schema
Application preferences now have an explicit schema version. Volatile window/session/cache/print state can be migrated or discarded independently of durable preferences such as language.

## Command palette
Ctrl+K opens a searchable command palette for frequently used commands such as Open, Save, Print, Find, Compare, OCR, Optimize, Preferences and Privacy.

## Performance and privacy
The existing lazy PDF loading, bounded LRU render cache, low-memory mode and local-only processing model are retained. Optional qpdf, Tesseract and LibreOffice integrations remain runtime-discovered and are not linked into the lightweight core.
