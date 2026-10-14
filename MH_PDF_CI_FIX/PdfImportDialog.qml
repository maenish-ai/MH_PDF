import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Dialog {
 id: dialog
 title: win.tx("import.title")
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
  Label { text: win.tx("import.url_label") }
  TextField { id: address; Layout.fillWidth: true; placeholderText: win.tx("import.url_hint") }
  RowLayout {
   Button { text: win.tx("import.find"); enabled: !importer.busy; onClicked: importer.inspect(address.text) }
   Button { text: win.tx("import.download"); enabled: !importer.busy; onClicked: importer.download(address.text) }
   Button { text: win.tx("import.cancel"); enabled: importer.busy; onClicked: importer.cancel() }
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
  Label { text: win.tx("import.access_note"); wrapMode: Text.Wrap }
 }
 Connections {
  target: importer
  function onCandidatesFound(items) { dialog.candidates = items }
  function onDownloaded(path) { if (documents.openDocument(path)) dialog.close() }
  function onFailed(message) { errorText.text = message; errorDialog.open() }
 }
 Dialog { id: errorDialog; title: win.tx("import.error_title"); modal: true; anchors.centerIn: parent; standardButtons: Dialog.Ok
  Label { id: errorText; width: 350; wrapMode: Text.Wrap }
 }
}
