from pathlib import Path
import sys,re
r=Path(__file__).resolve().parents[1]
required=['core/IPdfBackend.h','core/QtPdfBackend.h','core/QtPdfBackend.cpp','core/MemoryPolicy.h','core/MemoryPolicy.cpp','core/PageImageProvider.h','core/PageImageProvider.cpp','core/LanguageManager.h','core/LanguageManager.cpp']
checks={f'file:{f}':(r/f).exists() for f in required}
cpp=(r/'core/PdfDocument.cpp').read_text(encoding='utf-8')
h=(r/'core/PdfDocument.h').read_text(encoding='utf-8')
pm=(r/'core/PageModel.cpp').read_text(encoding='utf-8')
cap=(r/'core/EngineCapabilities.cpp').read_text(encoding='utf-8')
load=cpp[cpp.index('bool PdfDocument::loadIntoModel'):cpp.index('bool PdfDocument::openDocument')]
checks.update({
 'engine v6 name':'MaenPDF Engine 6' in h,
 'backend abstraction':'std::make_shared<QtPdfBackend>()' in cpp and 'QHash<QString, std::shared_ptr<IPdfBackend>>' in h,
 'lazy open does not render':'.render(' not in load and 'sourcePage = pageIndex' in load,
 'LRU cache':'m_lru' in pm and 'm_cacheBudget' in pm and 'putCache' in pm,
 'cache bounded by memory policy':'MemoryPolicy::renderCacheBudgetBytes()' in pm,
 'page image provider':'image://maenpdf/page/' in pm,
 'search API':'searchText' in h and 'pageText' in cpp,
 'Android content URI':'isContentUri' in cpp and 'prepareReadablePath' in cpp,
 'honest structural capability':'structuralTextEditing' in cap and '= false' in cap,
 'honest OCR capability':'ocr' in cap and '= false' in cap,
 'bilingual capability':'bilingualUi' in cap,
})
for n,v in checks.items(): print(('PASS' if v else 'FAIL'),n)
sys.exit(0 if all(checks.values()) else 1)
