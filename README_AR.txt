MaenPDF 7.5.5 — Import PDF UI + Background Download Upgrade

التعديلات:
1. نافذة Import PDF أصبحت نافذة Windows مستقلة قابلة للتصغير والتكبير والإغلاق.
2. إضافة CheckBox بجانب كل PDF واختيار متعدد.
3. Select all / Clear selection.
4. عرض اسم الكتاب/الملف بخط واضح، وتحت الاسم الموقع والرابط.
5. استخراج أسماء أفضل من نصوص روابط HTML و query parameters عندما تكون متاحة.
6. اكتشاف embedded application/pdf حتى لو الرابط لا ينتهي بـ .pdf.
7. إغلاق نافذة Import لا يوقف التنزيل.
8. على Desktop: إذا أغلقت نافذة MaenPDF الرئيسية أثناء تنزيل فعّال، يستمر التنزيل في الخلفية مع System Tray، ثم يغلق البرنامج بعد انتهاء قائمة التنزيل. الضغط على أيقونة MaenPDF في Tray يعيد فتح البرنامج.
9. الحماية الحالية من localhost/private network redirects بقيت موجودة، ولا يوجد تجاوز لتسجيل دخول/DRM/صلاحيات المواقع.

الملفات التي تستبدلها في المستودع:
- core/WebPdfImporter.h
- core/WebPdfImporter.cpp
- ui/PdfImportDialog.qml
- tests/import_batch_selection_audit.py

رقم الإصدار بقي 7.5.5، لأن المشروع الحالي بالفعل مضبوط على 7.5.5 ولم يتم إنشاء Release النهائي بعد.

ملاحظة:
بقية Dialogs الموجودة داخل Main.qml ما زالت Qt Quick popups. هذا التعديل يجعل Import PDF نافذة Native كاملة الآن؛ ويمكن اعتماد نفس النمط للنوافذ الكبيرة الأخرى (Search/Compare/Report/Preferences/Properties) في تحديث لاحق بدون كسر الحوارات الصغيرة مثل Save/Password/Unsaved warnings.
