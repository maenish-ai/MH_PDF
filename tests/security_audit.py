from pathlib import Path
import re,sys
r=Path(__file__).resolve().parents[1]
source='\n'.join(p.read_text(encoding='utf-8',errors='ignore') for folder in ['app','core','ui'] for p in (r/folder).rglob('*') if p.is_file())
gitignore=(r/'.gitignore').read_text(encoding='utf-8')
tools=(r/'core/PdfToolsService.cpp').read_text(encoding='utf-8')
checks={
 'no document network client':'QNetworkAccessManager' not in source and 'QNetworkRequest' not in source,
 'no embedded private key':'BEGIN PRIVATE KEY' not in source and 'BEGIN RSA PRIVATE KEY' not in source,
 'no shell command execution':'cmd.exe' not in tools and 'powershell' not in tools and 'system(' not in tools,
 'external providers use QProcess arguments':'process.start(program, arguments)' in tools,
 'provider timeouts':'waitForFinished(timeoutMs)' in tools and 'process.kill()' in tools,
 'OCR language validation':'languagePattern' in tools,
 'signing secrets ignored':'*.jks' in gitignore and '*.keystore' in gitignore,
 'security policy exists':(r/'SECURITY.md').exists(),
 'brand signing policy':'Official signing keys' in (r/'BRAND_POLICY.md').read_text(encoding='utf-8'),
 'no cloud rule documented':'does not upload user PDFs' in (r/'SECURITY.md').read_text(encoding='utf-8'),
}
for n,v in checks.items(): print(('PASS' if v else 'FAIL'),n)
sys.exit(0 if all(checks.values()) else 1)
