import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: dialog
    visible: false
    title: tx("web_pdf.title")
    width: 980
    height: 680
    minimumWidth: 720
    minimumHeight: 480
    flags: Qt.Window
           | Qt.WindowTitleHint
           | Qt.WindowSystemMenuHint
           | Qt.WindowMinimizeButtonHint
           | Qt.WindowMaximizeButtonHint
           | Qt.WindowCloseButtonHint

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
    property bool resetting: false
    property var selectedUrls: ({})
    property int selectedCount: 0

    function open() {
        show()
        raise()
        requestActivate()
    }

    function clearSelection() {
        selectedUrls = ({})
        selectedCount = 0
    }

    function isSelected(url) {
        return selectedUrls[url] === true
    }

    function setSelected(url, checked) {
        var next = ({})
        var count = 0
        for (var key in selectedUrls) {
            if (selectedUrls[key] === true && key !== url) {
                next[key] = true
                count++
            }
        }
        if (checked) {
            next[url] = true
            count++
        }
        selectedUrls = next
        selectedCount = count
    }

    function selectAll() {
        var next = ({})
        for (var i = 0; i < candidates.length; ++i) {
            var url = candidates[i].url
            if (url)
                next[url] = true
        }
        selectedUrls = next
        selectedCount = candidates.length
    }

    function resetDialog() {
        batchActive = false
        queue = []
        queueIndex = 0
        succeeded = 0
        failedCount = 0
        failures = []
        suppressAutoOpen = true
        resetting = true
        clearSelection()
        batchSummary.close()
        errorDialog.close()
        importer.resetUi()
        address.text = ""
        candidates = []
        resetting = false
    }

    function beginSelected() {
        if (importer.busy || candidates.length === 0 || selectedCount === 0)
            return

        var items = []
        var seen = ({})
        for (var i = 0; i < candidates.length; ++i) {
            var url = candidates[i].url
            if (url && selectedUrls[url] === true && !seen[url]) {
                seen[url] = true
                items.push(url)
            }
        }

        if (items.length === 0)
            return

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
        if (!batchActive)
            return

        if (queueIndex >= queue.length) {
            batchActive = false
            batchSummary.open()
            return
        }

        importer.download(queue[queueIndex])
    }

    function completeItem(ok, detail) {
        if (!batchActive)
            return

        if (ok)
            succeeded++
        else {
            failedCount++
            failures.push(detail)
        }

        queueIndex++
        Qt.callLater(startNext)
    }

    onClosing: function(closeEvent) {
        // X hides this tool window but never kills an active download.
        // The queue keeps running in the application background.
        closeEvent.accepted = false
        hide()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Label {
            text: tx("web_pdf.url_label")
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: address
                Layout.fillWidth: true
                enabled: !dialog.batchActive && !importer.busy
                placeholderText: tx("web_pdf.url_placeholder")
                selectByMouse: true
                onTextEdited: {
                    dialog.candidates = []
                    dialog.clearSelection()
                }
                Keys.onReturnPressed: {
                    if (!importer.busy && !dialog.batchActive) {
                        dialog.candidates = []
                        dialog.clearSelection()
                        dialog.suppressAutoOpen = false
                        importer.inspect(address.text)
                    }
                }
            }

            Button {
                text: tx("web_pdf.find")
                enabled: !importer.busy && !dialog.batchActive
                onClicked: {
                    dialog.candidates = []
                    dialog.clearSelection()
                    dialog.suppressAutoOpen = false
                    importer.inspect(address.text)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: i18n.rtl ? "تحديد الكل" : "Select all"
                enabled: dialog.candidates.length > 0
                         && !importer.busy && !dialog.batchActive
                onClicked: dialog.selectAll()
            }

            Button {
                text: i18n.rtl ? "إلغاء التحديد" : "Clear selection"
                enabled: dialog.selectedCount > 0
                         && !importer.busy && !dialog.batchActive
                onClicked: dialog.clearSelection()
            }

            Item { Layout.fillWidth: true }

            Label {
                text: i18n.rtl
                      ? ("تم تحديد " + dialog.selectedCount + " من " + dialog.candidates.length)
                      : (dialog.selectedCount + " of " + dialog.candidates.length + " selected")
                font.bold: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: dialog.candidates.length > 0
                      ? tx("web_pdf.download_selected")
                      : tx("web_pdf.download")
                enabled: !importer.busy && !dialog.batchActive
                         && (dialog.candidates.length === 0 || dialog.selectedCount > 0)
                highlighted: dialog.selectedCount > 0
                onClicked: {
                    if (dialog.candidates.length > 0) {
                        dialog.beginSelected()
                    } else if (/\.pdf(?:[?#]|$)/i.test(address.text)) {
                        dialog.suppressAutoOpen = false
                        importer.download(address.text)
                    } else {
                        dialog.candidates = []
                        dialog.clearSelection()
                        importer.inspect(address.text)
                    }
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
                enabled: !dialog.batchActive
                onClicked: dialog.resetDialog()
            }

            Item { Layout.fillWidth: true }

            Label {
                visible: importer.busy || dialog.batchActive
                text: i18n.rtl
                      ? "يمكنك إغلاق هذه النافذة وسيستمر التنزيل في الخلفية"
                      : "You may close this window; downloads continue in the background."
                opacity: 0.75
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: 100
            value: dialog.resetting ? 0 : importer.progress
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: dialog.batchActive
                  ? tx("web_pdf.downloading_n_of_m",
                       [dialog.queueIndex + 1, dialog.queue.length])
                    + " — " + importer.status
                  : importer.status
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            border.width: 1
            border.color: "#cbd5e1"
            color: "transparent"

            ListView {
                id: resultsList
                anchors.fill: parent
                anchors.margins: 6
                model: dialog.candidates
                clip: true
                spacing: 4

                ScrollBar.vertical: ScrollBar { }

                delegate: Rectangle {
                    required property var modelData
                    required property int index

                    width: resultsList.width
                    height: 76
                    radius: 7
                    color: dialog.isSelected(modelData.url)
                           ? (dialog.palette.highlight.lighter(1.85))
                           : (index % 2 === 0 ? dialog.palette.base : dialog.palette.window)
                    border.width: dialog.isSelected(modelData.url) ? 1 : 0
                    border.color: dialog.palette.highlight

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10

                        CheckBox {
                            id: selector
                            enabled: !importer.busy && !dialog.batchActive
                            checked: dialog.isSelected(modelData.url)
                            onToggled: dialog.setSelected(modelData.url, checked)
                            Accessible.name: modelData.title
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3

                            Label {
                                Layout.fillWidth: true
                                text: modelData.title || "PDF"
                                font.bold: true
                                font.pixelSize: 15
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.host || ""
                                opacity: 0.72
                                elide: Text.ElideRight
                                visible: text.length > 0
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.url
                                opacity: 0.62
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                            }
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: dialog.candidates.length === 0 && !importer.busy
                    text: i18n.rtl
                          ? "ستظهر ملفات PDF التي يتم العثور عليها هنا"
                          : "Discovered PDF files will appear here."
                    opacity: 0.55
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: tx("web_pdf.notice")
            wrapMode: Text.Wrap
            opacity: 0.75
        }
    }

    Connections {
        target: importer

        function onCandidatesFound(items) {
            if (!dialog.resetting) {
                dialog.candidates = items
                dialog.clearSelection()
                if (items.length === 1)
                    dialog.setSelected(items[0].url, true)
            }
        }

        function onDownloaded(path) {
            if (dialog.batchActive) {
                dialog.completeItem(true, path)
                return
            }

            if (dialog.suppressAutoOpen)
                return

            if (documents.openDocument(path)) {
                homeVisible = false
                dialog.hide()
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
        parent: dialog.contentItem
        title: tx("web_pdf.download_completed")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok

        Label {
            width: 390
            wrapMode: Text.Wrap
            text: tx("web_pdf.batch_result",
                     [dialog.succeeded, dialog.failedCount])
        }
    }

    Dialog {
        id: errorDialog
        parent: dialog.contentItem
        title: tx("web_pdf.error_title")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok

        Label {
            id: errorText
            width: 430
            wrapMode: Text.Wrap
        }
    }
}
