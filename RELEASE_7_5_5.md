# MaenPDF 7.5.5

Maintenance release focused on Windows distribution trust and release integrity.

- Official Windows release pipeline requires Authenticode signing credentials.
- `MaenPDF.exe` is signed before it is packaged.
- `MaenPDF-Setup.exe` is signed after Inno Setup packaging.
- The signed installer and installed executable are verified before publication.
- SHA-256 release manifest is generated from the final signed installer.
- Windows, runtime, installer, and Android package version metadata are synchronized at 7.5.5.
- Existing 7.5.4 installations can discover 7.5.5 through the existing GitHub Releases updater.

No antivirus bypass, packing, obfuscation, or detection-evasion mechanism is introduced.
