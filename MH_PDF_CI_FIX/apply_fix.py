from pathlib import Path
import json, shutil, sys
repo=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path.cwd()
source=Path(__file__).parent
qml=repo/'ui/PdfImportDialog.qml'
if not qml.exists(): raise SystemExit('Missing ui/PdfImportDialog.qml; choose MH_PDF repository root')
shutil.copyfile(source/'PdfImportDialog.qml',qml)
translations={
'import.title':('Smart PDF Import','استيراد ملفات بي دي إف الذكي'),
'import.url_label':('PDF or webpage URL','رابط ملف أو صفحة ويب'),
'import.url_hint':('Paste a web link here','ألصق رابط الموقع هنا'),
'import.find':('Find PDFs','البحث عن ملفات بي دي إف'),
'import.download':('Download','تحميل'),
'import.cancel':('Cancel','إلغاء'),
'import.access_note':('Accessible PDFs only. DRM and access restrictions are not bypassed.','الملفات المتاحة فقط، ولا يتم تجاوز الحماية أو قيود الوصول.'),
'import.error_title':('Import error','خطأ في الاستيراد')}
for lang,index in [('en',0),('ar',1)]:
    path=repo/f'i18n/{lang}.json'
    data=json.loads(path.read_text(encoding='utf-8'))
    for key,values in translations.items():data[key]=values[index]
    path.write_text(json.dumps(dict(sorted(data.items())),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Patched QML and both language catalogs. Run python tests/localization_audit.py')
