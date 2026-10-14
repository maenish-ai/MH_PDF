import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Dialog {
 id: dialog
 title: "Smart PDF Import / استيراد PDF"
 modal: true
 width: Math.min(680, parent ? parent.width-40 : 680)
 height: 440
 anchors.centerIn: parent
 standardButtons: Dialog.Close
 property var importer
 property var documents
 property var candidates: []
 ColumnLayout {
  anchors.fill: parent
  Label { text: "PDF or webpage URL / رابط الملف أو الصفحة" }
  TextField { id: address; Layout.fillWidth: true; placeholderText: "https://example.org/file.pdf" }
  RowLayout {
   Button { text: "Find PDFs / فحص"; enabled: !importer.busy; onClicked: importer.inspect(address.text) }
   Button { text: "Download / تحميل"; enabled: !importer.busy; onClicked: importer.download(address.text) }
   Button { text: "Cancel"; enabled: importer.busy; onClicked: importer.cancel() }
  }
  ProgressBar { Layout.fillWidth: true; from: 0; to: 100; value: importer.progress }
  Label { text: importer.status; wrapMode: Text.Wrap }
  ListView {
   Layout.fillWidth: true; Layout.fillHeight: true; model: dialog.candidates; clip: true
   delegate: ItemDelegate {
    width: ListView.view.width
    text: modelData.title + " — " + modelData.url
    onClicked: importer.download(modelData.url)
   }
  }
  Label { text: "Accessible PDFs only. DRM and access restrictions are not bypassed."; wrapMode: Text.Wrap }
 }
 Connections {
  target: importer
  function onCandidatesFound(items) { dialog.candidates = items }
  function onDownloaded(path) { if (documents.openDocument(path)) dialog.close() }
  function onFailed(message) { errorText.text = message; errorDialog.open() }
 }
 Dialog { id: errorDialog; title: "Import error"; modal: true; anchors.centerIn: parent; standardButtons: Dialog.Ok
  Label { id: errorText; width: 350; wrapMode: Text.Wrap }
 }
}
