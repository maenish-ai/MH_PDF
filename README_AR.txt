هذا تصحيح الشارة الحمراء في GitHub Actions.
فشل فحص الترجمة بسبب Download completed.
شغّل داخل مجلد المشروع بعد فك الضغط:
python fix.py /path/to/MH_PDF
ثم ارفع الملفات الثلاثة التي تغيرت في commit واحد:
ui/PdfImportDialog.qml
i18n/en.json
i18n/ar.json
هذا ليس Setup ولا APK. سيعيد GitHub Actions البناء بعد رفع التعديلات.
