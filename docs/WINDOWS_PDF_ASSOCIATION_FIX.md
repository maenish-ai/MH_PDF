# Windows PDF association fix

Setup now registers MaenPDF machine-wide in Open With and Default Apps using Applications, SupportedTypes, RegisteredApplications and Capabilities/FileAssociations, alongside MaenPDF.Document and .pdf/OpenWithProgids. ChangesAssociations notifies Explorer. Uninstall removes only application-owned registrations. Existing user defaults are preserved.

Launch paths use QCoreApplication::arguments() to preserve Windows Unicode filenames, including Arabic.

Build the Windows Setup workflow, download and install the new Setup, then right-click a PDF, choose Open with, select MaenPDF and enable Always. Uploading source alone does not update an installed application.

Local source checks pass; installation and Explorer behavior require Windows validation.

Reference: https://learn.microsoft.com/en-us/windows/win32/shell/default-programs
