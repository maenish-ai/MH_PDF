#!/usr/bin/env python3
import json, sys
from pathlib import Path
repo=Path(sys.argv[1] if len(sys.argv)>1 else ".")
qml=repo/"ui/PdfImportDialog.qml"
if not qml.is_file(): raise SystemExit("Run: python fix.py /path/to/MH_PDF")
s=qml.read_text(encoding="utf-8")
changes={
 'title: "Download completed"':'title: tx("web_pdf.download_completed")',
 '"Download All (" + dialog.candidates.length + ")"':'tx("web_pdf.download_all", [dialog.candidates.length])',
 '"Downloading " + (dialog.queueIndex + 1) + " of " + dialog.queue.length':'tx("web_pdf.downloading_n_of_m", [dialog.queueIndex + 1, dialog.queue.length])',
 'dialog.succeeded + " PDFs downloaded successfully, "\n                  + dialog.failedCount + " failed.\\nSaved in your Downloads folder."':'tx("web_pdf.batch_result", [dialog.succeeded, dialog.failedCount])'
}
for before,after in changes.items():
 if before not in s and after not in s: raise SystemExit("Expected QML expression not found: "+before)
 s=s.replace(before,after)
qml.write_text(s,encoding="utf-8")
translations={
"en":{"web_pdf.download_completed":"Download completed","web_pdf.download_all":"Download All (%1)","web_pdf.downloading_n_of_m":"Downloading %1 of %2","web_pdf.batch_result":"%1 PDFs downloaded successfully, %2 failed.\nSaved in your Downloads folder."},
"ar":{"web_pdf.download_completed":"اكتمل التنزيل","web_pdf.download_all":"تحميل الكل (%1)","web_pdf.downloading_n_of_m":"جار تنزيل %1 من %2","web_pdf.batch_result":"تم تنزيل %1 من ملفات بي دي إف بنجاح، وفشل تنزيل %2.\nحُفظت الملفات في مجلد التنزيلات."}}
for lang,items in translations.items():
 p=repo/"i18n"/(lang+".json")
 obj=json.loads(p.read_text(encoding="utf-8"))
 obj.update(items)
 p.write_text(json.dumps(dict(sorted(obj.items())),ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
 print("Updated",p)
print("Updated",qml)
