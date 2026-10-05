# MaenPDF PDF Compatibility Target

MaenPDF aims for broad standards-based PDF interoperability rather than claiming that every possible PDF file can be opened without exception.

## Target

- Common PDF 1.x files and modern PDF 2.0 constructs supported by the active Qt PDF/PDFium reader.
- Documents produced by major desktop/mobile/browser/office/scanner workflows.
- English, Arabic and mixed-direction documents.
- Password-protected PDFs supported by the reader's security schemes.

## Regression corpus plan

The project should maintain legal-to-redistribute samples covering:

- generated and scanned PDFs;
- Arabic/English/mixed text;
- bookmarks, links, images and unusual page sizes;
- password-protected files;
- PDF/A, PDF/X and PDF/UA samples where available;
- forms/signatures as read-only compatibility samples until authoring/signing is implemented;
- malformed/truncated/adversarial files;
- large documents and image-heavy documents.

Every fixed compatibility bug should add a non-sensitive reproducible regression sample where licensing permits.

## Non-goal

A corrupted file, proprietary extension, unsupported encryption method or implementation defect may still fail. The application must report failure safely rather than pretending successful parsing.
