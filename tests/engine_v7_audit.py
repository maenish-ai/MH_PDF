from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
required=['core/IPdfBackend.h','core/QtPdfBackend.h','core/QtPdfBackend.cpp','core/MemoryPolicy.h','core/MemoryPolicy.cpp','core/DocumentManager.h','core/DocumentManager.cpp','core/SessionImageProvider.h','core/SessionImageProvider.cpp','core/PdfToolsService.h','core/PdfToolsService.cpp','core/AppSettings.h','core/AppSettings.cpp','core/PrintService.h','core/PrintService.cpp']
checks={f'file:{f}':(r/f).exists() for f in required}
cpp=(r/'core/PdfDocument.cpp').read_text(encoding='utf-8')
h=(r/'core/PdfDocument.h').read_text(encoding='utf-8')
pm=(r/'core/PageModel.cpp').read_text(encoding='utf-8')
cap=(r/'core/EngineCapabilities.cpp').read_text(encoding='utf-8')
tools=(r/'core/PdfToolsService.cpp').read_text(encoding='utf-8')
dm=(r/'core/DocumentManager.cpp').read_text(encoding='utf-8')
checks.update({
 'engine v7 name':'MaenPDF Engine 7' in h,
 'backend abstraction':'std::make_shared<QtPdfBackend>()' in cpp and 'QHash<QString, std::shared_ptr<IPdfBackend>>' in h,
 'lazy open source-backed':'sourcePage = pageIndex' in cpp,
 'LRU cache':'m_lru' in pm and 'm_cacheBudget' in pm and 'putCache' in pm,
 'cache bounded by memory policy':'MemoryPolicy::renderCacheBudgetBytes()' in pm,
 'session image provider':'image://maenpdf/%1/page/' in pm,
 'multi-document tabs':'m_documents' in dm and 'currentDocumentChanged' in dm,
 'close other tabs':'closeOtherTabs' in dm,
 'settings schema migration':'CurrentSettingsSchema' in (r/'core/AppSettings.h').read_text(encoding='utf-8') and 'migrateSettings' in (r/'core/AppSettings.cpp').read_text(encoding='utf-8'),
 'native print service':'QPrintDialog' in (r/'core/PrintService.cpp').read_text(encoding='utf-8') and 'QPrintPreviewDialog' in (r/'core/PrintService.cpp').read_text(encoding='utf-8'),
 'search API':'searchText' in h and 'pageText' in cpp,
 'Android content URI':'isContentUri' in cpp and 'prepareReadablePath' in cpp,
 'password open':'openDocumentWithPassword' in cpp and 'IncorrectPassword' in (r/'core/QtPdfBackend.cpp').read_text(encoding='utf-8'),
 'local PDF compare':'comparePdf' in tools and 'differencePercent' in tools,
 'local OCR provider':'ocrToSearchablePdf' in tools and 'tesseract' in tools and 'mergePdfs' in tools,
 'qpdf provider':'optimizePdf' in tools and 'decryptPdf' in tools and 'splitPdf' in tools,
 'LibreOffice bridge':'officeToPdf' in tools and 'soffice' in tools,
 'safe flatten':'safeFlattenPdf' in tools and 'writeFlattened' in tools,
 'image export':'exportImages' in tools,
 'honest structural capability':'structuralTextEditing' in cap and '= false' in cap,
 'honest signature capability':'digitalSignatures' in cap and '= false' in cap,
 'honest forms capability':'acroFormAuthoring' in cap and '= false' in cap,
 'secure flattened redaction capability':'secureFlattenedRedaction' in cap and 'addRedaction' in cpp,
 'bilingual capability':'bilingualUi' in cap,
 'no cloud capability':'localOnlyProcessing' in cap and 'cloudUpload' in cap and '= false' in cap,
})
for n,v in checks.items(): print(('PASS' if v else 'FAIL'),n)
sys.exit(0 if all(checks.values()) else 1)
