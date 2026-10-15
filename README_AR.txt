MaenPDF 7.5.5 hotfix — Import PDF + GitHub Actions

ما الذي تم إصلاحه؟

1) Import PDF from Internet
- المشكلة التي ظهرت مع رابط Scribd ليست crash في MaenPDF.
- الرابط الذي أُدخل هو صفحة ويب/عارض JavaScript وليس رابط PDF مباشر.
- النسخة السابقة كانت تنهي الفحص برسالة مضللة: "Select a PDF..." حتى لو كانت القائمة فارغة.
- التعديل الجديد:
  * يوضح صراحة عندما لا يوجد PDF مباشر.
  * يتعرف على روابط PDF العادية.
  * يتعرف أيضًا على موارد HTML المعلنة بـ type="application/pdf" حتى لو لم ينته الرابط بـ .pdf.
  * يضيف User-Agent خاص بالبرنامج لتحسين التوافق مع خوادم الويب.
  * لا يتجاوز تسجيل الدخول أو DRM أو صلاحيات مواقع الويب.

2) علامة GitHub Actions الحمراء
- فحصت آخر تشغيل فعلي.
- MaenPDF CI نفسه نجح بالكامل (Windows + Android + audits).
- الفشل كان فقط في Publish Signed MaenPDF Release.
- السبب الدقيق: هذان الـ GitHub Secrets غير موجودين:
    WINDOWS_SIGNING_PFX_BASE64
    WINDOWS_SIGNING_PFX_PASSWORD
- auto-release.yml المعدل هنا لا يحول نقص شهادة التوقيع إلى خطأ أحمر:
  * CI يبقى ناجحًا.
  * Release الموقّع يتم تخطيه بأمان مع Warning واضح.
  * لن ينشر أي Windows Release غير موقع.
  * بعد إضافة الـ secrets شغّل workflow يدويًا وسيتم التوقيع والنشر.

طريقة الرفع:
- فك ضغط ZIP.
- ارفع المجلدين .github و core إلى جذر المستودع مع الحفاظ على نفس المسارات.
- وافق على استبدال الملفين الموجودين.
- لا ترفع أي ملف PFX إلى المستودع.

مهم:
لا يمكن إنشاء توقيع Authenticode موثوق من كود عادي. يلزم شهادة Code Signing موثوقة ومفتاحها الخاص داخل GitHub Secrets.
