إصلاح فشل بناء Windows وAndroid في GitHub Actions رقم 38553008675.
السبب: استدعاء qBound بقيم عددية غير متطابقة في core/WebPdfImporter.cpp السطر 81.
التعديل: تحديد نوع qint64 صراحة لكل من الحد الأدنى والحد الأعلى.
ارفع ملف core/WebPdfImporter.cpp فقط إلى المسار نفسه في مستودع GitHub، مع استبدال النسخة القديمة.
بعد الرفع راقب GitHub Actions؛ لم يتم التحقق من البناء الفعلي محلياً.
