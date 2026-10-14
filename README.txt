Upload .github/workflows/auto-release.yml into the same repository path.
On the GitHub Actions tab, run "Publish MaenPDF Release" manually once after a successful CI run. Future successful main CI runs publish only when the version has changed.
IMPORTANT: This workflow publishes only the Windows installer. Android CI APK is signed with an ephemeral test key and MUST NOT be released as a production APK. A separate signed Android release workflow with persistent secrets is required.
If GitHub Actions cannot publish, check Settings > Actions > General > Workflow permissions > Read and write permissions.
