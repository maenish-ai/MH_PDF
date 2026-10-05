from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
license_text=(r/'LICENSE').read_text(encoding='utf-8',errors='ignore')
brand=(r/'BRAND_POLICY.md').read_text(encoding='utf-8')
checks={
 'GPL license present':'GNU GENERAL PUBLIC LICENSE' in license_text and 'Version 3' in license_text,
 'brand policy present':'MaenPDF Brand Policy' in brand,
 'contributors can be credited':'Contributors may add their name' in (r/'CONTRIBUTING.md').read_text(encoding='utf-8'),
 'product name protected separately':'brand policy is separate' in brand.lower(),
 'SBOM present':(r/'SBOM.md').exists(),
 'security policy present':(r/'SECURITY.md').exists(),
}
for n,v in checks.items(): print(('PASS' if v else 'FAIL'),n)
sys.exit(0 if all(checks.values()) else 1)
