# GitHub CI fix — QtPdf extension

Qt PDF is distributed as a Qt extension in modern Qt releases. The previous workflow used
`jurplel/install-qt-action` with `modules: qtpdf`, which can fail with the upstream error
`The packages ['qtpdf'] were not found while parsing XML of package information`.

The v6.0.1 workflow bypasses that failing integration path and invokes the current aqtinstall
source directly. Qt 6.11.2 is fully pinned and QtPdf is explicitly installed for Windows,
Android arm64-v8a, and Android armeabi-v7a. Each build verifies `Qt6PdfConfig.cmake` before
running CMake so a dependency-install problem is reported clearly before compilation.

If GitHub later ships a fixed stable aqtinstall/install-qt-action combination, this workaround
can be replaced with a pinned stable release after CI validation.
