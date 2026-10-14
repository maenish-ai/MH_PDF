import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog
    title: tx("web_pdf.title")
    modal: true
    width: Math.min(740, parent ? parent.width - 40 : 740)
    height: 500
    anchors.centerIn: parent
    standardButtons: importer.busy || batchActive ? Dialog.NoButton : Dialog.Close
    property var importer
    property var documents
    property var candidates: []
    property bool batchActive: false
    property var queue: []
    property int queueIndex: 0
    property int succeeded: 0
    property int failedCount: 0
    property var failures: []
    property bool suppressAutoOpen: false
    property int selectedIndex: -1
    property bool resetting: false

    function resetDialog() {
        // Invalidate the queue before aborting the network request.
        batchActive = false
        queue = []
        queueIndex = 0
        succeeded = 0
        failedCount = 0
        failures = []
        selectedIndex = -1
        suppressAutoOpen = true
        resetting = true
        batchSummary.close()
        errorDialog.close()
        importer.resetUi()
        address.text = ""
        candidates = []
        resetting = false
    }


    function beginAll() {
        if (importer.busy || candidates.length === 0) return
        var seen = {}
        var items = []
        for (var i = 0; i < candidates.length; ++i) {
            var url = candidates[i].url
            if (url && !seen[url]) { seen[url] = true; items.push(url) }
        }
        if (items.length === 0) return
        selectedIndex = -1
        queue = items
        queueIndex = 0
        succeeded = 0
        failedCount = 0
        failures = []
        batchActive = true
        suppressAutoOpen = true
        startNext()
    }
    function startNext() {
        if (!batchActive) return
        if (queueIndex >= queue.length) {
            batchActive = false
            batchSummary.open()
            return
        }
        importer.download(queue[queueIndex])
    }
    function completeItem(ok, detail) {
        if (!batchActive) return
        if (ok) succeeded++
        else {
            failedCount++
            failures.push(detail)
        }
        queueIndex++
        // Defer until WebPdfImporter has set busy=false after its signal.
        Qt.callLater(startNext)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        Label { text: tx("web_pdf.url_label") }
        TextField {
            id: address
            Layout.fillWidth: true
            enabled: !dialog.batchActive && !importer.busy
            placeholderText: tx("web_pdf.url_placeholder")
            onTextEdited: { dialog.candidates = []; dialog.selectedIndex = -1 }
        }
        RowLayout {
            Button {
                text: tx("web_pdf.find")
                enabled: !importer.busy && !dialog.batchActive
                onClicked: { dialog.candidates = []; dialog.selectedIndex = -1; dialog.suppressAutoOpen = false; importer.inspect(address.text) }
            }
            Button {
                text: dialog.candidates.length > 0
                      ? (dialog.selectedIndex >= 0 ? tx("web_pdf.download_selected") : tx("web_pdf.download_all", [dialog.candidates.length]))
                      : tx("web_pdf.download")
                enabled: !importer.busy && !dialog.batchActive
                onClicked: {
                    if (dialog.candidates.length > 0) {
                        if (dialog.selectedIndex >= 0 && dialog.selectedIndex < dialog.candidates.length) {
                            dialog.suppressAutoOpen = true
                            importer.download(dialog.candidates[dialog.selectedIndex].url)
                        } else dialog.beginAll()
                    }
                    else if (/\.pdf(?:[?#]|$)/i.test(address.text)) importer.download(address.text)
                    else { dialog.candidates = []; importer.inspect(address.text) }
                }
            }
            Button {
                text: tx("web_pdf.cancel")
                enabled: importer.busy || dialog.batchActive
                onClicked: {
                    if (dialog.batchActive) {
                        dialog.batchActive = false
                        dialog.suppressAutoOpen = true
                    }
                    importer.cancel()
                }
            }
            Button {
                text: tx("web_pdf.reset")
                onClicked: dialog.resetDialog()
            }
        }
        ProgressBar { Layout.fillWidth: true; from: 0; to: 100; value: dialog.resetting ? 0 : importer.progress }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: dialog.batchActive
                  ? tx("web_pdf.downloading_n_of_m", [dialog.queueIndex + 1, dialog.queue.length])
                    + " — " + importer.status
                  : importer.status
        }
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: dialog.candidates
            clip: true
            delegate: ItemDelegate {
                width: ListView.view.width
                highlighted: dialog.selectedIndex === index
                text: (dialog.selectedIndex === index ? "✓ " : "") + modelData.title + " — " + modelData.url
                enabled: !importer.busy && !dialog.batchActive
                onClicked: {
                    dialog.selectedIndex = dialog.selectedIndex === index ? -1 : index
                }
            }
        }
        Label { text: tx("web_pdf.notice"); wrapMode: Text.Wrap }
    }
    Connections {
        target: importer
        function onCandidatesFound(items) { if (!dialog.resetting) { dialog.candidates = items; dialog.selectedIndex = -1 } }
        function onDownloaded(path) {
            if (dialog.batchActive) {
                dialog.completeItem(true, path)
                return
            }
            if (dialog.suppressAutoOpen) return
            if (documents.openDocument(path)) {
                homeVisible = false
                dialog.close()
            }
        }
        function onFailed(message) {
            if (dialog.batchActive) {
                dialog.completeItem(false, message)
                return
            }
            if (!dialog.suppressAutoOpen) {
                errorText.text = message
                errorDialog.open()
            }
        }
    }
    Dialog {
        id: batchSummary
        title: tx("web_pdf.download_completed")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok
        Label {
            width: 350
            wrapMode: Text.Wrap
            text: tx("web_pdf.batch_result", [dialog.succeeded, dialog.failedCount])
        }
    }
    Dialog {
        id: errorDialog
        title: tx("web_pdf.error_title")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok
        Label { id: errorText; width: 350; wrapMode: Text.Wrap }
    }
}
