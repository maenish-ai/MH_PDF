import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: win
    width: 1500
    height: 920
    minimumWidth: 980
    minimumHeight: 650
    visible: true
    color: appSettings.darkMode ? "#111827" : "#eef1f5"
    title: (pdfDocument && pdfDocument.modified ? "• " : "") + displayDocumentTitle() + " — " + tx("app.window_title")
    LayoutMirroring.enabled: i18n.rtl
    LayoutMirroring.childrenInherit: true

    property var pdfDocument: documentManager.currentDocument
    property real zoom: 0.74
    property string tool: "select"
    property string selectedText: ""
    property var selectedTextRects: []
    property int selectedTextPage: -1
    property int selectedAnnotationPage: -1
    property string selectedAnnotationId: ""
    property int pendingActionPage: -1
    property string pendingActionTool: ""
    property var pendingActionRect: ({"x": 0, "y": 0, "w": 0, "h": 0})
    property color drawColor: "#2563eb"
    property real drawWidth: 3.0
    property real minInkStepPx: appSettings.lowMemoryMode ? 4.0 : 2.5
    property int drawOpacity: 100
    property color highlightColor: "#ffd54f"
    property int highlightOpacity: 42
    property bool syncingPageFromScroll: false
    property bool homeVisible: true
    property bool allowWindowClose: false
    property int pendingCloseTab: -1
    property string pendingAfterSave: ""
    property string passwordPath: ""
    property var compareResults: []
    property string toolReport: ""
    property string pendingImagePath: ""
    property var providersState: pdfTools.providers
    property bool showSidebar: true
    property var documentProperties: ({})

    readonly property color panelColor: appSettings.darkMode ? "#1f2937" : "#ffffff"
    readonly property color subPanelColor: appSettings.darkMode ? "#18212f" : "#f8f9fb"
    readonly property color canvasColor: appSettings.darkMode ? "#0b1020" : "#d9dde3"
    readonly property color textColor: appSettings.darkMode ? "#f3f4f6" : "#172033"
    readonly property color mutedColor: appSettings.darkMode ? "#aeb8c8" : "#667085"
    readonly property color borderColor: appSettings.darkMode ? "#334155" : "#dce1e8"
    readonly property color accentColor: "#2864dc"
    readonly property color accentViolet: "#7c3aed"
    readonly property color accentCyan: "#0891b2"
    readonly property color accentEmerald: "#059669"
    readonly property color accentAmber: "#d97706"
    readonly property color pointerColor: "#2864dc"
    readonly property color textToolColor: "#7c3aed"
    readonly property color highlightToolColor: "#d97706"
    readonly property color drawToolColor: "#0891b2"
    readonly property color redactToolColor: "#dc2626"
    readonly property color cropToolColor: "#059669"

    palette.window: panelColor
    palette.windowText: textColor
    palette.button: panelColor
    palette.buttonText: textColor
    palette.highlight: accentColor
    palette.text: textColor
    palette.base: subPanelColor

    function tx(key, args) {
        var revisionDependency = i18n.revision
        var value = i18n.t(key)
        if (args) {
            for (var i = 0; i < args.length; ++i)
                value = value.replace("%" + (i + 1), args[i])
        }
        return value
    }

    function displayDocumentTitle() {
        if (!pdfDocument)
            return tx("document.untitled")
        return pdfDocument.filePath === "" ? tx("document.untitled") : pdfDocument.title
    }

    function translatedMessage(key, args) {
        var converted = []
        if (args) {
            for (var i = 0; i < args.length; ++i)
                converted.push(typeof args[i] === "number" ? i18n.number(args[i]) : args[i])
        }
        return tx(key, converted)
    }

    function showToast(key, args) {
        toast.text = translatedMessage(key, args || [])
        toast.open()
    }

    function fitWidth() {
        if (!pdfDocument || !documentView)
            return
        var w = pdfDocument.pages.pageWidth(pdfDocument.currentPage)
        zoom = Math.max(0.12, Math.min(3.0, (documentView.width - 90) / Math.max(1, w)))
    }

    function fitPage() {
        if (!pdfDocument || !documentView)
            return
        var w = pdfDocument.pages.pageWidth(pdfDocument.currentPage)
        var h = pdfDocument.pages.pageHeight(pdfDocument.currentPage)
        zoom = Math.max(0.12, Math.min(3.0, Math.min((documentView.width - 90) / Math.max(1, w), (documentView.height - 70) / Math.max(1, h))))
    }

    function zoomIn() { zoom = Math.min(4.0, Math.round((zoom + 0.1) * 100) / 100) }
    function zoomOut() { zoom = Math.max(0.12, Math.round((zoom - 0.1) * 100) / 100) }

    function navigateToPage(index, positionMode) {
        if (!pdfDocument || !documentView || pdfDocument.pageCount <= 0)
            return
        var target = Math.max(0, Math.min(pdfDocument.pageCount - 1, index))
        pdfDocument.currentPage = target
        Qt.callLater(function() {
            if (documentView)
                documentView.positionViewAtIndex(target, positionMode === undefined ? ListView.Beginning : positionMode)
        })
        homeVisible = false
    }

    function scrollByScreen(direction) {
        if (!documentView)
            return
        var amount = Math.max(120, documentView.height * 0.88)
        var maxY = Math.max(0, documentView.contentHeight - documentView.height)
        documentView.contentY = Math.max(0, Math.min(maxY, documentView.contentY + direction * amount))
    }

    function clearOverlaySelection() {
        selectedAnnotationPage = -1
        selectedAnnotationId = ""
    }

    function deleteCurrentSelection() {
        if (selectedAnnotationPage >= 0 && selectedAnnotationId !== "") {
            pdfDocument.deleteTextAnnotation(selectedAnnotationPage, selectedAnnotationId)
            clearOverlaySelection()
            return
        }
        if (selectedText !== "") {
            clearTextSelection()
            return
        }
    }

    function keyboardTextInputActive() {
        var item = win.activeFocusItem
        return item && typeof item.cursorPosition !== "undefined"
    }

    function canUseSingleKeyShortcut() {
        return appSettings.singleKeyShortcuts && !keyboardTextInputActive()
    }

    function performanceProfileLabel() {
        if (appSettings.performanceProfile === "eco")
            return tx("performance.eco")
        if (appSettings.performanceProfile === "performance")
            return tx("performance.performance")
        return tx("performance.balanced")
    }

    function requireProvider(name) {
        if (providersState && providersState[name])
            return true
        providersDlg.open()
        return false
    }

    function clearTextSelection() {
        selectedText = ""
        selectedTextRects = []
        selectedTextPage = -1
    }

    function cancelPendingAction() {
        pendingActionPage = -1
        pendingActionTool = ""
        pendingActionRect = ({"x": 0, "y": 0, "w": 0, "h": 0})
    }

    function chooseTool(name) {
        if (tool !== name) {
            cancelPendingAction()
            if (name !== "select") {
                clearTextSelection()
                clearOverlaySelection()
            }
        }
        tool = name
        homeVisible = false
    }

    function applyPendingAction() {
        if (pendingActionPage < 0)
            return
        var r = pendingActionRect
        if (pendingActionTool === "crop")
            pdfDocument.cropPage(pendingActionPage, r.x, r.y, r.w, r.h)
        else if (pendingActionTool === "redact")
            pdfDocument.addRedaction(pendingActionPage, r.x, r.y, r.w, r.h)
        cancelPendingAction()
        chooseTool("select")
    }

    function toolHint() {
        if (pendingActionPage >= 0)
            return pendingActionTool === "crop" ? tx("tool.crop_ready") : tx("tool.redact_ready")
        if (tool === "hand") return tx("tool.hand_hint")
        if (tool === "text") return tx("tool.text_hint")
        if (tool === "highlight") return tx("tool.highlight_hint")
        if (tool === "draw") return tx("tool.draw_hint")
        if (tool === "redact") return tx("tool.redact_hint")
        if (tool === "crop") return tx("tool.crop_hint")
        return selectedText !== "" ? tx("tool.selection_ready") : tx("tool.pointer_hint")
    }

    function requestCloseTab(index) {
        documentManager.currentIndex = index
        if (pdfDocument && pdfDocument.modified) {
            pendingCloseTab = index
            closeTabDlg.open()
        } else {
            documentManager.closeTab(index)
        }
    }

    function finishCloseTabAfterSave() {
        if (pendingCloseTab >= 0) {
            var index = pendingCloseTab
            pendingCloseTab = -1
            documentManager.closeTab(index)
        }
    }

    onClosing: function(close) {
        if (pdfTools.busy) {
            close.accepted = false
            showToast("status.wait_operation", [])
            return
        }
        if (!allowWindowClose && documentManager.hasModifiedDocuments) {
            close.accepted = false
            closeAllDlg.open()
        }
    }

    Component.onCompleted: {
        if (pdfDocument && pdfDocument.recoveryAvailable)
            recoveryDlg.open()
    }

    Connections {
        target: documentManager
        function onCurrentDocumentChanged() {
            tool = "select"
            clearTextSelection()
            clearOverlaySelection()
            cancelPendingAction()
            zoom = 0.74
            if (pdfDocument && pdfDocument.recoveryAvailable)
                recoveryDlg.open()
        }
    }

    Connections {
        target: pdfDocument
        function onErrorOccurred(key, args) { showToast(key, args) }
        function onInfo(key, args) { showToast(key, args) }
        function onPasswordRequired(path) {
            passwordPath = path
            passwordInput.clear()
            passwordDlg.open()
        }
    }

    Connections {
        target: pdfTools
        function onOperationFinished(key, args) { showToast(key, args) }
        function onOperationFailed(key, args) { showToast(key, args) }
        function onProvidersChanged() { providersState = pdfTools.providers }
        function onCompareReady(results) { compareResults = results; compareResultDlg.open() }
        function onReportReady(report) { toolReport = report; reportDlg.open() }
    }

    Connections {
        target: printService
        function onOperationFinished(key, args) { showToast(key, args) }
        function onOperationFailed(key, args) { showToast(key, args) }
    }

    Connections {
        target: appSettings
        function onLowMemoryModeChanged() { if (pdfDocument) pdfDocument.refreshMemoryPolicy() }
        function onAdaptivePerformanceChanged() { if (pdfDocument) pdfDocument.refreshMemoryPolicy() }
    }

    Rectangle {
        anchors.fill: parent
        z: 900
        visible: pdfTools.busy
        color: appSettings.darkMode ? "#66000000" : "#55ffffff"
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons }
        Rectangle {
            anchors.centerIn: parent
            width: 300
            height: 110
            radius: 14
            color: panelColor
            border.color: borderColor
            RowLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14
                BusyIndicator { running: pdfTools.busy }
                ColumnLayout {
                    Label { text: tx("status.processing"); font.bold: true; color: textColor }
                    Label { text: pdfTools.currentOperation; color: mutedColor; Layout.maximumWidth: 210; elide: Text.ElideRight }
                    Label { text: tx("status.ui_responsive"); color: mutedColor; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.maximumWidth: 210 }
                }
            }
        }
    }

    DropArea {
        anchors.fill: parent
        z: 1000
        onDropped: function(drop) {
            if (!drop.hasUrls)
                return
            for (var i = 0; i < drop.urls.length; ++i) {
                var url = drop.urls[i].toString()
                if (url.toLowerCase().endsWith(".pdf"))
                    documentManager.openDocument(url)
            }
            homeVisible = false
            drop.acceptProposedAction()
        }
    }

    FileDialog {
        id: openDlg
        title: tx("file.open_pdf")
        nameFilters: [tx("file.pdf_filter")]
        onAccepted: {
            documentManager.openDocument(selectedFile)
            homeVisible = false
        }
    }

    FileDialog {
        id: appendDlg
        title: tx("file.combine_pdf")
        nameFilters: [tx("file.pdf_filter")]
        onAccepted: pdfDocument.appendPdf(selectedFile)
    }

    FileDialog {
        id: saveDlg
        title: tx("file.save_pdf")
        fileMode: FileDialog.SaveFile
        nameFilters: [tx("file.pdf_filter")]
        defaultSuffix: "pdf"
        onAccepted: {
            if (pdfDocument.saveAs(selectedFile)) {
                if (pendingAfterSave === "closeTab")
                    finishCloseTabAfterSave()
                pendingAfterSave = ""
            }
        }
    }

    FileDialog {
        id: extractDlg
        title: tx("file.extract_page")
        fileMode: FileDialog.SaveFile
        nameFilters: [tx("file.pdf_filter")]
        defaultSuffix: "pdf"
        onAccepted: pdfDocument.extractPage(pdfDocument.currentPage, selectedFile)
    }

    FileDialog {
        id: imageDlg
        title: tx("file.insert_image")
        nameFilters: [tx("file.image_filter")]
        onAccepted: pdfDocument.addImage(pdfDocument.currentPage, selectedFile)
    }

    FileDialog {
        id: signatureDlg
        title: tx("file.insert_signature")
        nameFilters: [tx("file.image_filter")]
        onAccepted: pdfDocument.addImage(pdfDocument.currentPage, selectedFile, 0.56, 0.72, 0.28, 0.16)
    }

    FileDialog {
        id: protectDlg
        title: tx("file.create_protected")
        fileMode: FileDialog.SaveFile
        nameFilters: [tx("file.pdf_filter")]
        defaultSuffix: "pdf"
        onAccepted: pdfDocument.protectCopy(selectedFile, userPass.text, ownerPass.text,
                                             allowPrint.checked, allowCopy.checked, allowModify.checked)
    }

    FileDialog {
        id: compareDlg
        title: tx("file.compare_with")
        nameFilters: [tx("file.pdf_filter")]
        onAccepted: pdfTools.startComparePdf(pdfDocument.filePath, selectedFile, 0)
    }

    FileDialog {
        id: toolOutputDlg
        property string operation: ""
        title: tx("file.choose_output")
        fileMode: FileDialog.SaveFile
        nameFilters: [tx("file.pdf_filter")]
        defaultSuffix: "pdf"
        onAccepted: {
            if (operation === "optimize")
                pdfTools.startOptimizePdf(pdfDocument.filePath, selectedFile)
            else if (operation === "linearize")
                pdfTools.startLinearizePdf(pdfDocument.filePath, selectedFile)
            else if (operation === "repair")
                pdfTools.startRepairPdf(pdfDocument.filePath, selectedFile)
            else if (operation === "safeFlatten")
                pdfTools.startSafeFlattenPdf(pdfDocument.filePath, selectedFile, 150)
            else if (operation === "ocr")
                pdfTools.startOcrToSearchablePdf(pdfDocument.filePath, selectedFile, ocrLanguages.text, ocrDpi.value)
            else if (operation === "decrypt")
                pdfTools.startDecryptPdf(pdfDocument.filePath, selectedFile, decryptPassword.text)
        }
    }

    FolderDialog {
        id: imagesFolderDlg
        title: tx("file.choose_folder")
        onAccepted: pdfTools.startExportImages(pdfDocument.filePath, selectedFolder, exportDpi.value)
    }

    FolderDialog {
        id: splitFolderDlg
        title: tx("file.choose_folder")
        onAccepted: pdfTools.startSplitPdf(pdfDocument.filePath, selectedFolder, splitPages.value)
    }

    FileDialog {
        id: batchOptimizeFilesDlg
        title: tx("file.batch_optimize")
        fileMode: FileDialog.OpenFiles
        nameFilters: [tx("file.pdf_filter")]
        onAccepted: {
            batchOptimizeFolderDlg.inputs = selectedFiles
            batchOptimizeFolderDlg.open()
        }
    }

    FolderDialog {
        id: batchOptimizeFolderDlg
        property var inputs: []
        title: tx("file.choose_folder")
        onAccepted: {
            var paths = []
            for (var i = 0; i < inputs.length; ++i)
                paths.push(inputs[i].toString())
            pdfTools.startBatchOptimize(paths, selectedFolder)
            inputs = []
        }
        onRejected: inputs = []
    }

    FileDialog {
        id: imageToPdfInputDlg
        title: tx("file.image_to_pdf")
        nameFilters: [tx("file.image_filter")]
        onAccepted: {
            pendingImagePath = selectedFile
            imageToPdfOutputDlg.open()
        }
    }

    FileDialog {
        id: imageToPdfOutputDlg
        title: tx("file.choose_output")
        fileMode: FileDialog.SaveFile
        nameFilters: [tx("file.pdf_filter")]
        defaultSuffix: "pdf"
        onAccepted: pdfTools.startImageToPdf(pendingImagePath, selectedFile)
    }

    FileDialog {
        id: officeInputDlg
        title: tx("file.office_to_pdf")
        nameFilters: [tx("file.office_filter")]
        onAccepted: officeFolderDlg.pendingOffice = selectedFile
    }

    FolderDialog {
        id: officeFolderDlg
        property string pendingOffice: ""
        title: tx("file.choose_folder")
        onPendingOfficeChanged: {
            if (pendingOffice !== "")
                open()
        }
        onAccepted: {
            pdfTools.startOfficeToPdf(pendingOffice, selectedFolder)
            pendingOffice = ""
        }
        onRejected: pendingOffice = ""
    }

    Dialog {
        id: passwordDlg
        title: tx("dialog.password_title")
        modal: true
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            Label { text: tx("dialog.password_required"); wrapMode: Text.WordWrap; Layout.preferredWidth: 420 }
            TextField { id: passwordInput; placeholderText: tx("dialog.password"); echoMode: TextInput.Password; Layout.fillWidth: true; onAccepted: passwordOk.clicked() }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: passwordDlg.reject() }
            Button {
                id: passwordOk
                text: tx("dialog.open")
                highlighted: true
                onClicked: {
                    if (documentManager.openDocumentWithPassword(passwordPath, passwordInput.text))
                        passwordDlg.accept()
                }
            }
        }
    }

    Dialog {
        id: closeTabDlg
        title: tx("dialog.unsaved_title")
        modal: true
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            Label { text: tx("dialog.unsaved_tab"); wrapMode: Text.WordWrap; Layout.preferredWidth: 440 }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: { pendingCloseTab = -1; closeTabDlg.reject() } }
            Button {
                text: tx("dialog.discard")
                onClicked: {
                    var index = pendingCloseTab
                    pdfDocument.newDocument()
                    pendingCloseTab = -1
                    documentManager.closeTab(index)
                    closeTabDlg.accept()
                }
            }
            Button {
                text: tx("dialog.save")
                highlighted: true
                onClicked: {
                    if (pdfDocument.filePath === "") {
                        pendingAfterSave = "closeTab"
                        saveDlg.open()
                    } else if (pdfDocument.save()) {
                        finishCloseTabAfterSave()
                    }
                    closeTabDlg.accept()
                }
            }
        }
    }

    Dialog {
        id: closeAllDlg
        title: tx("dialog.unsaved_title")
        modal: true
        closePolicy: Popup.NoAutoClose
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            Label { text: tx("dialog.unsaved_all"); wrapMode: Text.WordWrap; Layout.preferredWidth: 470 }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: closeAllDlg.reject() }
            Button {
                text: tx("dialog.discard_all")
                onClicked: {
                    allowWindowClose = true
                    closeAllDlg.accept()
                    Qt.quit()
                }
            }
        }
    }

    Dialog {
        id: textDlg
        title: tx("dialog.insert_text")
        modal: true
        standardButtons: Dialog.NoButton
        property real nx: 0.15
        property real ny: 0.2
        ColumnLayout {
            anchors.fill: parent
            TextField { id: textInput; placeholderText: tx("dialog.enter_text"); Layout.preferredWidth: 380 }
            RowLayout {
                Label { text: tx("dialog.size") }
                SpinBox { id: fontSize; from: 8; to: 96; value: 18 }
            }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: textDlg.reject() }
            Button { text: tx("dialog.ok"); highlighted: true; onClicked: textDlg.accept() }
        }
        onAccepted: {
            var createdId = pdfDocument.addText(pdfDocument.currentPage, nx, ny, textInput.text, fontSize.value)
            if (createdId !== "") {
                selectedAnnotationPage = pdfDocument.currentPage
                selectedAnnotationId = createdId
                clearTextSelection()
                // Return to Pointer mode so the new text is visibly selected and
                // can be removed immediately with Delete/Backspace or the toolbar button.
                tool = "select"
            }
            textInput.clear()
        }
    }

    Dialog {
        id: waterDlg
        title: tx("dialog.watermark")
        modal: true
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            TextField { id: waterText; placeholderText: tx("dialog.watermark_placeholder"); Layout.preferredWidth: 380 }
            RowLayout {
                Label { text: tx("dialog.opacity") }
                Slider { id: waterOpacity; from: 10; to: 90; value: 35; Layout.fillWidth: true }
            }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: waterDlg.reject() }
            Button { text: tx("dialog.ok"); highlighted: true; onClicked: waterDlg.accept() }
        }
        onAccepted: pdfDocument.addWatermark(waterText.text, 42, waterOpacity.value)
    }

    Dialog {
        id: batesDlg
        title: tx("dialog.bates_title")
        modal: true
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            TextField { id: batesPrefix; placeholderText: tx("dialog.bates_prefix"); Layout.preferredWidth: 360 }
            RowLayout {
                Label { text: tx("dialog.bates_start") }
                SpinBox { id: batesStart; from: 0; to: 9999999; value: 1 }
            }
            RowLayout {
                Label { text: tx("dialog.bates_padding") }
                SpinBox { id: batesPadding; from: 1; to: 12; value: 6 }
            }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: batesDlg.reject() }
            Button { text: tx("dialog.apply"); highlighted: true; onClicked: batesDlg.accept() }
        }
        onAccepted: pdfDocument.addBatesNumbers(batesPrefix.text, batesStart.value, batesPadding.value)
    }

    Dialog {
        id: securityDlg
        title: tx("dialog.protect")
        modal: true
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            Label { text: tx("dialog.encrypted_interoperable"); font.bold: true }
            TextField { id: userPass; placeholderText: tx("dialog.open_password"); echoMode: TextInput.Password; Layout.preferredWidth: 420 }
            TextField { id: ownerPass; placeholderText: tx("dialog.owner_password"); echoMode: TextInput.Password }
            CheckBox { id: allowPrint; text: tx("dialog.allow_printing"); checked: true }
            CheckBox { id: allowCopy; text: tx("dialog.allow_copying") }
            CheckBox { id: allowModify; text: tx("dialog.allow_modifications") }
            Label { text: tx("dialog.security_provider"); color: mutedColor }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: securityDlg.reject() }
            Button { text: tx("dialog.save"); highlighted: true; enabled: providersState.qpdf; onClicked: securityDlg.accept() }
        }
        onAccepted: protectDlg.open()
    }

    Dialog {
        id: decryptDlg
        title: tx("dialog.decrypt_title")
        modal: true
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            TextField { id: decryptPassword; placeholderText: tx("dialog.password"); echoMode: TextInput.Password; Layout.preferredWidth: 380 }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: decryptDlg.reject() }
            Button {
                text: tx("dialog.continue")
                highlighted: true
                onClicked: {
                    toolOutputDlg.operation = "decrypt"
                    decryptDlg.accept()
                    toolOutputDlg.open()
                }
            }
        }
    }

    Dialog {
        id: ocrDlg
        title: tx("dialog.ocr_title")
        modal: true
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            Label { text: tx("dialog.ocr_local_note"); wrapMode: Text.WordWrap; Layout.preferredWidth: 460 }
            TextField { id: ocrLanguages; text: pdfTools.defaultOcrLanguages; Layout.fillWidth: true }
            RowLayout {
                Label { text: tx("dialog.dpi") }
                SpinBox { id: ocrDpi; from: 100; to: 300; value: 150 }
            }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: ocrDlg.reject() }
            Button {
                text: tx("dialog.create")
                highlighted: true
                enabled: providersState.qpdf && providersState.tesseract
                onClicked: {
                    toolOutputDlg.operation = "ocr"
                    ocrDlg.accept()
                    toolOutputDlg.open()
                }
            }
        }
    }

    Dialog {
        id: exportImagesDlg
        title: tx("dialog.export_images_title")
        modal: true
        standardButtons: Dialog.NoButton
        RowLayout {
            anchors.fill: parent
            Label { text: tx("dialog.dpi") }
            SpinBox { id: exportDpi; from: 72; to: 300; value: 144 }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: exportImagesDlg.reject() }
            Button { text: tx("dialog.choose_folder"); highlighted: true; onClicked: { exportImagesDlg.accept(); imagesFolderDlg.open() } }
        }
    }

    Dialog {
        id: splitDlg
        title: tx("dialog.split_title")
        modal: true
        standardButtons: Dialog.NoButton
        RowLayout {
            anchors.fill: parent
            Label { text: tx("dialog.pages_per_file") }
            SpinBox { id: splitPages; from: 1; to: 100; value: 1 }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: splitDlg.reject() }
            Button { text: tx("dialog.choose_folder"); highlighted: true; onClicked: { splitDlg.accept(); splitFolderDlg.open() } }
        }
    }

    Dialog {
        id: recoveryDlg
        title: tx("dialog.recovery_title")
        modal: true
        closePolicy: Popup.NoAutoClose
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            spacing: 12
            Label { text: tx("dialog.recovery_found"); wrapMode: Text.WordWrap; Layout.preferredWidth: 430 }
            Label { text: tx("dialog.recovery_choice"); color: mutedColor; wrapMode: Text.WordWrap }
            RowLayout {
                Button { text: tx("dialog.recover"); highlighted: true; onClicked: { pdfDocument.recoverAutosave(); recoveryDlg.close(); homeVisible = false } }
                Button { text: tx("dialog.discard"); onClicked: { pdfDocument.discardRecovery(); recoveryDlg.close() } }
            }
        }
    }

    Dialog {
        id: searchDlg
        title: tx("dialog.find_title")
        modal: true
        standardButtons: Dialog.NoButton
        property var hits: []
        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            RowLayout {
                Layout.fillWidth: true
                TextField { id: searchInput; placeholderText: tx("dialog.search_text"); Layout.preferredWidth: 420; onAccepted: findBtn.clicked() }
                Button { id: findBtn; text: tx("dialog.find"); onClicked: searchDlg.hits = pdfDocument.searchText(searchInput.text, 200) }
            }
            Label { text: tx("dialog.results", [i18n.number(searchDlg.hits.length)]); color: mutedColor }
            ListView {
                Layout.preferredWidth: 560
                Layout.preferredHeight: 360
                clip: true
                model: searchDlg.hits
                delegate: ItemDelegate {
                    required property var modelData
                    width: ListView.view.width
                    text: tx("dialog.page_result", [i18n.number(modelData.page + 1), modelData.excerpt])
                    onClicked: { pdfDocument.currentPage = modelData.page; searchDlg.close(); homeVisible = false }
                }
            }
            Label { text: tx("dialog.search_note"); color: mutedColor; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
        footer: DialogButtonBox { Button { text: tx("dialog.close"); onClicked: searchDlg.close() } }
    }

    Dialog {
        id: compareResultDlg
        title: tx("dialog.compare_results")
        modal: true
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            Label { text: tx("dialog.compare_count", [i18n.number(compareResults.length)]); color: mutedColor }
            ListView {
                Layout.preferredWidth: 520
                Layout.preferredHeight: 380
                model: compareResults
                clip: true
                delegate: ItemDelegate {
                    required property var modelData
                    width: ListView.view.width
                    text: tx("dialog.compare_row", [i18n.number(modelData.page + 1), i18n.decimal(modelData.differencePercent, 2)])
                }
            }
        }
        footer: DialogButtonBox { Button { text: tx("dialog.close"); onClicked: compareResultDlg.close() } }
    }

    Dialog {
        id: reportDlg
        title: tx("dialog.report_title")
        modal: true
        standardButtons: Dialog.NoButton
        ScrollView {
            width: 650
            height: 420
            TextArea { text: toolReport; readOnly: true; wrapMode: Text.WrapAnywhere }
        }
        footer: DialogButtonBox { Button { text: tx("dialog.close"); onClicked: reportDlg.close() } }
    }

    Dialog {
        id: providersDlg
        title: tx("dialog.providers_title")
        modal: true
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            Label { text: tx("provider.local_only"); font.bold: true }
            Label { text: tx("provider.security") + ": " + (providersState.qpdf ? tx("common.available") : tx("common.not_installed")) }
            Label { text: tx("provider.ocr") + ": " + (providersState.tesseract ? tx("common.available") : tx("common.not_installed")) }
            Label { text: tx("provider.office") + ": " + (providersState.libreOffice ? tx("common.available") : tx("common.not_installed")) }
            Label { text: tx("provider.optional_note"); color: mutedColor; wrapMode: Text.WordWrap; Layout.preferredWidth: 480 }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.refresh"); onClicked: pdfTools.refreshProviders() }
            Button { text: tx("dialog.close"); onClicked: providersDlg.close() }
        }
    }

    Dialog {
        id: privacyDlg
        title: tx("dialog.privacy_title")
        modal: true
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            Label { text: tx("privacy.heading"); font.pixelSize: 19; font.bold: true }
            Label { text: tx("privacy.body"); wrapMode: Text.WordWrap; Layout.preferredWidth: 500 }
            Label { text: tx("privacy.no_cloud"); font.bold: true; color: accentColor }
        }
        footer: DialogButtonBox { Button { text: tx("dialog.close"); onClicked: privacyDlg.close() } }
    }

    Dialog {
        id: aboutDlg
        title: tx("dialog.about_title")
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            Label { text: "◈"; font.pixelSize: 52; color: accentColor; Layout.alignment: Qt.AlignHCenter }
            Label { text: tx("dialog.about_product"); font.pixelSize: 22; font.bold: true; Layout.alignment: Qt.AlignHCenter }
            Label { text: tx("dialog.about_body"); horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; Layout.preferredWidth: 480 }
            Label { text: tx("dialog.about_license"); color: mutedColor; Layout.alignment: Qt.AlignHCenter }
            Button { text: tx("action.project_page"); Layout.alignment: Qt.AlignHCenter; onClicked: appSettings.openProjectPage() }
            Button { text: tx("action.support"); enabled: appSettings.supportAvailable; Layout.alignment: Qt.AlignHCenter; onClicked: appSettings.openSupportPage() }
            Label { visible: !appSettings.supportAvailable; text: tx("support.not_configured"); color: mutedColor; wrapMode: Text.WordWrap; Layout.preferredWidth: 440; horizontalAlignment: Text.AlignHCenter }
        }
        footer: DialogButtonBox { Button { text: tx("dialog.ok"); onClicked: aboutDlg.close() } }
    }

    Dialog {
        id: preferencesDlg
        title: tx("dialog.preferences_title")
        modal: true
        standardButtons: Dialog.NoButton
        width: 520
        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            CheckBox {
                text: tx("action.dark_mode")
                checked: appSettings.darkMode
                onToggled: appSettings.darkMode = checked
            }
            CheckBox {
                text: tx("action.adaptive_performance")
                checked: appSettings.adaptivePerformance
                onToggled: appSettings.adaptivePerformance = checked
            }
            Label {
                text: tx("preferences.performance_profile", [performanceProfileLabel(), i18n.number(appSettings.totalMemoryMB)])
                color: appSettings.performanceProfile === "eco" ? accentEmerald : appSettings.performanceProfile === "performance" ? accentViolet : accentColor
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            CheckBox {
                text: tx("action.low_memory")
                checked: appSettings.lowMemoryMode
                enabled: !appSettings.adaptivePerformance
                onToggled: appSettings.lowMemoryMode = checked
            }
            CheckBox {
                text: tx("action.single_key_shortcuts")
                checked: appSettings.singleKeyShortcuts
                onToggled: appSettings.singleKeyShortcuts = checked
            }
            Label {
                text: tx("preferences.shortcuts_note")
                color: mutedColor
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            CheckBox {
                text: tx("action.safe_graphics")
                checked: appSettings.safeGraphics
                onToggled: appSettings.safeGraphics = checked
            }
            Label {
                text: tx("preferences.safe_graphics_note")
                color: mutedColor
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                text: tx("preferences.schema", [i18n.number(appSettings.settingsSchemaVersion)])
                color: mutedColor
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            RowLayout {
                Button {
                    text: tx("home.clear_recent")
                    enabled: appSettings.recentFiles.length > 0
                    onClicked: appSettings.clearRecentFiles()
                }
                Button {
                    text: tx("action.reset_settings")
                    onClicked: {
                        appSettings.resetApplicationSettings(true)
                        showToast("info.settings_reset", [])
                    }
                }
            }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.close"); onClicked: preferencesDlg.close() }
        }
    }

    Dialog {
        id: propertiesDlg
        title: tx("dialog.properties_title")
        modal: true
        standardButtons: Dialog.NoButton
        width: 600
        onOpened: documentProperties = pdfDocument ? pdfDocument.properties() : ({})
        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            Label { text: tx("properties.name") + ": " + (documentProperties.title || ""); color: textColor; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: tx("properties.path") + ": " + (documentProperties.path || ""); color: mutedColor; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true }
            Label { text: tx("properties.pages") + ": " + i18n.number(documentProperties.pages || 0); color: textColor }
            Label { text: tx("properties.size") + ": " + i18n.number(documentProperties.sizeBytes || 0) + " " + tx("properties.bytes"); color: textColor }
            Label { text: tx("properties.format") + ": " + (documentProperties.format || ""); color: textColor }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.close"); onClicked: propertiesDlg.close() }
        }
    }

    Dialog {
        id: goToPageDlg
        title: tx("dialog.go_to_page")
        modal: true
        standardButtons: Dialog.NoButton
        width: 380
        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            Label { text: tx("dialog.page_number"); color: textColor }
            SpinBox {
                id: goToPageSpin
                from: 1
                to: Math.max(1, pdfDocument.pageCount)
                value: Math.max(1, pdfDocument.currentPage + 1)
                editable: true
                Layout.fillWidth: true
            }
        }
        onOpened: goToPageSpin.value = Math.max(1, pdfDocument.currentPage + 1)
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: goToPageDlg.reject() }
            Button { text: tx("dialog.go"); highlighted: true; onClicked: { navigateToPage(goToPageSpin.value - 1); goToPageDlg.accept() } }
        }
    }

    Dialog {
        id: shortcutsDlg
        title: tx("dialog.shortcuts_title")
        modal: true
        standardButtons: Dialog.NoButton
        width: 700
        height: Math.min(760, win.height - 80)
        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth
            ColumnLayout {
                width: parent.width
                spacing: 7
                Label { text: tx("shortcuts.note"); color: mutedColor; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Repeater {
                    model: [
                        ["Ctrl+O", tx("action.open")], ["Ctrl+S", tx("action.save")], ["Ctrl+P", tx("action.print")], ["Ctrl+F", tx("action.find")],
                        ["Ctrl+=", tx("action.zoom_in")], ["Ctrl+-", tx("action.zoom_out")], ["Ctrl+0", tx("action.fit_page")], ["Ctrl+1", tx("action.actual_size")], ["Ctrl+2", tx("action.fit_width")],
                        ["Ctrl+L", tx("action.full_screen")], ["Ctrl+K", tx("action.preferences")], ["Ctrl+Shift+P", tx("action.command_palette")], ["Ctrl+Shift+N", tx("action.go_to_page")],
                        ["Ctrl+Shift+D", tx("action.delete_page")], ["Ctrl+Shift+T", tx("action.add_blank")], ["Ctrl+Shift+I", tx("action.combine")], ["Ctrl+W", tx("action.close_tab")], ["Ctrl+Tab / Ctrl+Shift+Tab", tx("action.next_previous_tab")], ["F4", tx("action.sidebar")],
                        ["V", tx("action.pointer")], ["H", tx("action.hand")], ["T", tx("action.edit_text")], ["U", tx("action.highlight")], ["D", tx("action.draw")], ["C", tx("action.crop")], ["Shift+Y", tx("action.redact")],
                        ["Page Up / Page Down", tx("action.scroll_screen")], ["Ctrl+Page Up / Ctrl+Page Down", tx("action.previous_next_page")], ["Home / End", tx("action.first_last_page")],
                        ["Delete / Backspace", tx("action.delete_selected_object")]
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        height: 34
                        radius: 7
                        color: index % 2 === 0 ? subPanelColor : "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            Label { text: modelData[0]; font.bold: true; color: accentColor; Layout.preferredWidth: 210 }
                            Label { text: modelData[1]; color: textColor; Layout.fillWidth: true }
                        }
                    }
                }
            }
        }
        footer: DialogButtonBox { Button { text: tx("dialog.close"); onClicked: shortcutsDlg.close() } }
    }

    Dialog {
        id: commandDlg
        title: tx("dialog.command_title")
        modal: true
        standardButtons: Dialog.NoButton
        width: 560
        onOpened: {
            commandSearch.clear()
            commandSearch.forceActiveFocus()
        }
        ColumnLayout {
            anchors.fill: parent
            spacing: 6
            TextField {
                id: commandSearch
                placeholderText: tx("dialog.command_search")
                Layout.fillWidth: true
            }
            Button { text: tx("action.open"); Layout.fillWidth: true; visible: commandSearch.text === "" || text.toLowerCase().indexOf(commandSearch.text.toLowerCase()) >= 0; onClicked: { commandDlg.close(); openDlg.open() } }
            Button { text: tx("action.save"); Layout.fillWidth: true; visible: commandSearch.text === "" || text.toLowerCase().indexOf(commandSearch.text.toLowerCase()) >= 0; onClicked: { commandDlg.close(); pdfDocument.filePath === "" ? saveDlg.open() : pdfDocument.save() } }
            Button { text: tx("action.print"); Layout.fillWidth: true; enabled: printService.available; visible: commandSearch.text === "" || text.toLowerCase().indexOf(commandSearch.text.toLowerCase()) >= 0; onClicked: { commandDlg.close(); printService.printDocument(pdfDocument, false) } }
            Button { text: tx("action.find"); Layout.fillWidth: true; visible: commandSearch.text === "" || text.toLowerCase().indexOf(commandSearch.text.toLowerCase()) >= 0; onClicked: { commandDlg.close(); searchDlg.open() } }
            Button { text: tx("action.compare"); Layout.fillWidth: true; enabled: pdfDocument.filePath !== ""; visible: commandSearch.text === "" || text.toLowerCase().indexOf(commandSearch.text.toLowerCase()) >= 0; onClicked: { commandDlg.close(); compareDlg.open() } }
            Button { text: tx("action.ocr"); Layout.fillWidth: true; enabled: pdfDocument.filePath !== ""; visible: commandSearch.text === "" || text.toLowerCase().indexOf(commandSearch.text.toLowerCase()) >= 0; onClicked: { commandDlg.close(); if (requireProvider("qpdf") && requireProvider("tesseract")) ocrDlg.open() } }
            Button { text: tx("action.optimize"); Layout.fillWidth: true; enabled: pdfDocument.filePath !== ""; visible: commandSearch.text === "" || text.toLowerCase().indexOf(commandSearch.text.toLowerCase()) >= 0; onClicked: { commandDlg.close(); if (requireProvider("qpdf")) { toolOutputDlg.operation = "optimize"; toolOutputDlg.open() } } }
            Button { text: tx("action.safe_flatten"); Layout.fillWidth: true; enabled: pdfDocument.filePath !== ""; visible: commandSearch.text === "" || text.toLowerCase().indexOf(commandSearch.text.toLowerCase()) >= 0; onClicked: { commandDlg.close(); toolOutputDlg.operation = "safeFlatten"; toolOutputDlg.open() } }
            Button { text: tx("action.preferences"); Layout.fillWidth: true; visible: commandSearch.text === "" || text.toLowerCase().indexOf(commandSearch.text.toLowerCase()) >= 0; onClicked: { commandDlg.close(); preferencesDlg.open() } }
            Button { text: tx("action.privacy"); Layout.fillWidth: true; visible: commandSearch.text === "" || text.toLowerCase().indexOf(commandSearch.text.toLowerCase()) >= 0; onClicked: { commandDlg.close(); privacyDlg.open() } }
        }
        footer: DialogButtonBox { Button { text: tx("dialog.close"); onClicked: commandDlg.close() } }
    }

    // Acrobat-familiar document shortcuts plus a few cross-application
    // conventions. Single-key tool accelerators can be disabled in Preferences.
    Shortcut { sequences: [StandardKey.New]; onActivated: { documentManager.newTab(); homeVisible = false } }
    Shortcut { sequences: [StandardKey.Open]; onActivated: openDlg.open() }
    Shortcut { sequences: [StandardKey.Save]; onActivated: pdfDocument.filePath === "" ? saveDlg.open() : pdfDocument.save() }
    Shortcut { sequences: [StandardKey.Find]; onActivated: searchDlg.open() }
    Shortcut { sequences: [StandardKey.Print]; enabled: printService.available; onActivated: printService.printDocument(pdfDocument, false) }
    Shortcut { sequences: [StandardKey.Undo]; onActivated: if (pdfDocument.canUndo) pdfDocument.undo() }
    Shortcut { sequences: [StandardKey.Redo]; onActivated: if (pdfDocument.canRedo) pdfDocument.redo() }
    Shortcut { sequences: [StandardKey.Copy]; enabled: !keyboardTextInputActive(); onActivated: selectedText !== "" ? pdfDocument.copyTextToClipboard(selectedText) : pdfDocument.copyPage(pdfDocument.currentPage) }
    // Never steal Backspace/Delete from a TextField. These remove only the
    // selected MaenPDF overlay object while the document canvas has focus.
    Shortcut { sequence: "Delete"; enabled: !keyboardTextInputActive(); onActivated: deleteCurrentSelection() }
    Shortcut { sequence: "Backspace"; enabled: !keyboardTextInputActive(); onActivated: deleteCurrentSelection() }

    Shortcut { sequences: ["Ctrl+=", "Ctrl++"]; onActivated: zoomIn() }
    Shortcut { sequence: "Ctrl+-"; onActivated: zoomOut() }
    Shortcut { sequence: "Ctrl+0"; onActivated: fitPage() }
    Shortcut { sequence: "Ctrl+1"; onActivated: zoom = 1.0 }
    Shortcut { sequence: "Ctrl+2"; onActivated: fitWidth() }
    Shortcut { sequence: "Ctrl+L"; onActivated: win.visibility = win.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
    Shortcut { sequence: "Ctrl+K"; onActivated: preferencesDlg.open() }
    Shortcut { sequence: "Ctrl+Shift+P"; onActivated: commandDlg.open() }
    Shortcut { sequence: "Ctrl+Shift+N"; onActivated: goToPageDlg.open() }
    Shortcut { sequence: "Ctrl+Shift+D"; enabled: !pdfDocument.locked && pdfDocument.pageCount > 1; onActivated: pdfDocument.deletePage(pdfDocument.currentPage) }
    Shortcut { sequence: "Ctrl+Shift+T"; onActivated: pdfDocument.addBlankPage() }
    Shortcut { sequence: "Ctrl+Shift+I"; enabled: !pdfDocument.locked; onActivated: appendDlg.open() }
    Shortcut { sequence: "Ctrl+W"; onActivated: requestCloseTab(documentManager.currentIndex) }
    Shortcut { sequence: "Ctrl+Tab"; enabled: documentManager.count > 1 && !keyboardTextInputActive(); onActivated: documentManager.currentIndex = (documentManager.currentIndex + 1) % documentManager.count }
    Shortcut { sequence: "Ctrl+Shift+Tab"; enabled: documentManager.count > 1 && !keyboardTextInputActive(); onActivated: documentManager.currentIndex = (documentManager.currentIndex - 1 + documentManager.count) % documentManager.count }
    Shortcut { sequence: "F4"; onActivated: showSidebar = !showSidebar }

    Shortcut { sequence: "PageDown"; enabled: !keyboardTextInputActive(); onActivated: scrollByScreen(1) }
    Shortcut { sequence: "PageUp"; enabled: !keyboardTextInputActive(); onActivated: scrollByScreen(-1) }
    Shortcut { sequence: "Ctrl+PageDown"; onActivated: navigateToPage(pdfDocument.currentPage + 1) }
    Shortcut { sequence: "Ctrl+PageUp"; onActivated: navigateToPage(pdfDocument.currentPage - 1) }
    Shortcut { sequence: "Right"; enabled: !keyboardTextInputActive(); onActivated: navigateToPage(pdfDocument.currentPage + 1) }
    Shortcut { sequence: "Left"; enabled: !keyboardTextInputActive(); onActivated: navigateToPage(pdfDocument.currentPage - 1) }
    Shortcut { sequence: "Home"; enabled: !keyboardTextInputActive(); onActivated: navigateToPage(0) }
    Shortcut { sequence: "End"; enabled: !keyboardTextInputActive(); onActivated: navigateToPage(pdfDocument.pageCount - 1) }
    Shortcut { sequence: "Escape"; enabled: !keyboardTextInputActive(); onActivated: { cancelPendingAction(); clearTextSelection(); clearOverlaySelection(); chooseTool("select") } }

    Shortcut { sequence: "V"; enabled: canUseSingleKeyShortcut(); onActivated: chooseTool("select") }
    Shortcut { sequence: "H"; enabled: canUseSingleKeyShortcut(); onActivated: chooseTool("hand") }
    Shortcut { sequence: "T"; enabled: canUseSingleKeyShortcut() && !pdfDocument.locked; onActivated: chooseTool("text") }
    Shortcut { sequence: "U"; enabled: canUseSingleKeyShortcut() && !pdfDocument.locked; onActivated: chooseTool("highlight") }
    Shortcut { sequence: "D"; enabled: canUseSingleKeyShortcut() && !pdfDocument.locked; onActivated: chooseTool("draw") }
    Shortcut { sequence: "C"; enabled: canUseSingleKeyShortcut() && !pdfDocument.locked; onActivated: chooseTool("crop") }
    Shortcut { sequence: "Shift+Y"; enabled: canUseSingleKeyShortcut() && !pdfDocument.locked; onActivated: chooseTool("redact") }

    Popup {
        id: toast
        x: (win.width - width) / 2
        y: win.height - height - 45
        z: 2000
        padding: 14
        property string text: ""
        background: Rectangle { radius: 10; color: "#172033" }
        contentItem: Label { color: "white"; text: toast.text; wrapMode: Text.WordWrap; width: Math.min(620, implicitWidth) }
        Timer { running: toast.visible; interval: 3200; onTriggered: toast.close() }
    }

    menuBar: MenuBar {
        Menu {
            title: tx("menu.file")
            Action { text: tx("action.home"); onTriggered: homeVisible = true }
            Action { text: tx("action.new_tab"); onTriggered: { documentManager.newTab(); homeVisible = false } }
            Action { text: tx("action.open"); onTriggered: openDlg.open() }
            Action { text: tx("action.combine"); enabled: !pdfDocument.locked; onTriggered: appendDlg.open() }
            MenuSeparator {}
            Action { text: tx("action.save"); onTriggered: pdfDocument.filePath === "" ? saveDlg.open() : pdfDocument.save() }
            Action { text: tx("action.save_as"); onTriggered: saveDlg.open() }
            Menu {
                title: tx("action.export")
                Action { text: tx("action.extract"); onTriggered: extractDlg.open() }
                Action { text: tx("action.export_images"); enabled: pdfDocument.filePath !== ""; onTriggered: exportImagesDlg.open() }
                Action { text: tx("action.safe_flatten"); enabled: pdfDocument.filePath !== ""; onTriggered: { toolOutputDlg.operation = "safeFlatten"; toolOutputDlg.open() } }
            Action { text: tx("action.sanitize_privacy"); enabled: pdfDocument.filePath !== ""; onTriggered: { toolOutputDlg.operation = "safeFlatten"; toolOutputDlg.open() } }
            }
            MenuSeparator {}
            Action { text: tx("action.print"); enabled: printService.available; onTriggered: printService.printDocument(pdfDocument, false) }
            Action { text: tx("action.print_current"); enabled: printService.available; onTriggered: printService.printDocument(pdfDocument, true) }
            Action { text: tx("action.print_preview"); enabled: printService.available; onTriggered: printService.printPreview(pdfDocument, false) }
            MenuSeparator {}
            Action { text: tx("action.properties"); onTriggered: propertiesDlg.open() }
            Action { text: tx("action.close_tab"); onTriggered: requestCloseTab(documentManager.currentIndex) }
            Action { text: tx("action.exit"); onTriggered: win.close() }
        }

        Menu {
            title: tx("menu.edit")
            Action { text: tx("action.undo"); enabled: pdfDocument.canUndo; onTriggered: pdfDocument.undo() }
            Action { text: tx("action.redo"); enabled: pdfDocument.canRedo; onTriggered: pdfDocument.redo() }
            MenuSeparator {}
            Action { text: tx("action.copy_page"); onTriggered: pdfDocument.copyPage(pdfDocument.currentPage) }
            Action { text: tx("action.paste_page"); enabled: pdfDocument.hasPageClipboard; onTriggered: pdfDocument.pastePage(pdfDocument.currentPage) }
            Action { text: tx("action.delete_selected_object"); enabled: selectedAnnotationId !== "" || selectedText !== ""; onTriggered: deleteCurrentSelection() }
            Action { text: tx("action.delete_page"); enabled: !pdfDocument.locked && pdfDocument.pageCount > 1; onTriggered: pdfDocument.deletePage(pdfDocument.currentPage) }
            MenuSeparator {}
            Action { text: tx("action.find"); onTriggered: searchDlg.open() }
            Action { text: tx("action.preferences"); onTriggered: preferencesDlg.open() }
        }

        Menu {
            title: tx("menu.view")
            Action { text: tx("action.zoom_in"); onTriggered: zoom = Math.min(3, zoom + 0.1) }
            Action { text: tx("action.zoom_out"); onTriggered: zoom = Math.max(0.12, zoom - 0.1) }
            Action { text: tx("action.actual_size"); onTriggered: zoom = 1.0 }
            Action { text: tx("action.fit_width"); onTriggered: fitWidth() }
            Action { text: tx("action.fit_page"); onTriggered: fitPage() }
            Action { text: tx("action.go_to_page"); onTriggered: goToPageDlg.open() }
            MenuSeparator {}
            Action { text: tx("action.sidebar"); checkable: true; checked: showSidebar; onTriggered: showSidebar = checked }
            Action { text: tx("action.full_screen"); checkable: true; checked: win.visibility === Window.FullScreen; onTriggered: win.visibility = checked ? Window.FullScreen : Window.Windowed }
            MenuSeparator {}
            Action { text: tx("action.dark_mode"); checkable: true; checked: appSettings.darkMode; onTriggered: appSettings.darkMode = checked }
            Action { text: tx("action.adaptive_performance"); checkable: true; checked: appSettings.adaptivePerformance; onTriggered: appSettings.adaptivePerformance = checked }
            Action { text: tx("action.low_memory"); checkable: true; checked: appSettings.lowMemoryMode; enabled: !appSettings.adaptivePerformance; onTriggered: appSettings.lowMemoryMode = checked }
        }

        Menu {
            title: tx("menu.document")
            Action { text: tx("action.combine"); enabled: !pdfDocument.locked; onTriggered: appendDlg.open() }
            Action { text: tx("action.compare"); enabled: pdfDocument.filePath !== ""; onTriggered: compareDlg.open() }
            Action { text: tx("action.ocr"); enabled: pdfDocument.filePath !== ""; onTriggered: { if (requireProvider("qpdf") && requireProvider("tesseract")) ocrDlg.open() } }
            MenuSeparator {}
            Action { text: tx("action.optimize"); enabled: pdfDocument.filePath !== ""; onTriggered: { if (requireProvider("qpdf")) { toolOutputDlg.operation = "optimize"; toolOutputDlg.open() } } }
            Action { text: tx("action.linearize"); enabled: pdfDocument.filePath !== ""; onTriggered: { if (requireProvider("qpdf")) { toolOutputDlg.operation = "linearize"; toolOutputDlg.open() } } }
            Action { text: tx("action.repair"); enabled: pdfDocument.filePath !== ""; onTriggered: { toolOutputDlg.operation = "repair"; toolOutputDlg.open() } }
            Action {
                text: tx("action.check")
                enabled: pdfDocument.filePath !== ""
                onTriggered: pdfTools.startCheckPdf(pdfDocument.filePath)
            }
            Action { text: tx("action.properties"); onTriggered: propertiesDlg.open() }
        }

        Menu {
            title: tx("menu.pages")
            Action { text: tx("action.add_blank"); enabled: !pdfDocument.locked; onTriggered: pdfDocument.addBlankPage() }
            Action { text: tx("action.duplicate"); enabled: !pdfDocument.locked; onTriggered: pdfDocument.duplicatePage(pdfDocument.currentPage) }
            Action { text: tx("action.move_page_up"); enabled: !pdfDocument.locked && pdfDocument.currentPage > 0; onTriggered: pdfDocument.movePage(pdfDocument.currentPage, pdfDocument.currentPage - 1) }
            Action { text: tx("action.move_page_down"); enabled: !pdfDocument.locked && pdfDocument.currentPage < pdfDocument.pageCount - 1; onTriggered: pdfDocument.movePage(pdfDocument.currentPage, pdfDocument.currentPage + 1) }
            Action { text: tx("action.rotate_clockwise"); enabled: !pdfDocument.locked; onTriggered: pdfDocument.rotatePage(pdfDocument.currentPage, 90) }
            Action { text: tx("action.crop"); enabled: !pdfDocument.locked; onTriggered: chooseTool("crop") }
            Action { text: tx("action.extract"); onTriggered: extractDlg.open() }
            Action { text: tx("action.split"); enabled: pdfDocument.filePath !== ""; onTriggered: { if (requireProvider("qpdf")) splitDlg.open() } }
            MenuSeparator {}
            Action { text: tx("action.add_page_numbers"); enabled: !pdfDocument.locked; onTriggered: pdfDocument.addPageNumbers() }
            Action { text: tx("action.bates"); enabled: !pdfDocument.locked; onTriggered: batesDlg.open() }
            Action { text: tx("action.delete_page"); enabled: !pdfDocument.locked && pdfDocument.pageCount > 1; onTriggered: pdfDocument.deletePage(pdfDocument.currentPage) }
        }

        Menu {
            title: tx("menu.comment")
            Action { text: tx("action.pointer"); onTriggered: chooseTool("select") }
            Action { text: tx("action.hand"); onTriggered: chooseTool("hand") }
            Action { text: tx("action.edit_text"); enabled: !pdfDocument.locked; onTriggered: chooseTool("text") }
            Action { text: tx("action.highlight"); enabled: !pdfDocument.locked; onTriggered: chooseTool("highlight") }
            Action { text: tx("action.draw"); enabled: !pdfDocument.locked; onTriggered: chooseTool("draw") }
            Action { text: tx("action.insert_image"); enabled: !pdfDocument.locked; onTriggered: imageDlg.open() }
            Action { text: tx("action.signature"); enabled: !pdfDocument.locked; onTriggered: signatureDlg.open() }
            MenuSeparator {}
            Action { text: tx("action.watermark"); enabled: !pdfDocument.locked; onTriggered: waterDlg.open() }
            Action { text: tx("action.add_page_numbers"); enabled: !pdfDocument.locked; onTriggered: pdfDocument.addPageNumbers() }
        }

        Menu {
            title: tx("menu.forms")
            Action { text: tx("action.form_fill"); enabled: false }
            Action { text: tx("action.form_text_field"); enabled: false }
            Action { text: tx("action.form_checkbox"); enabled: false }
            Action { text: tx("action.form_radio"); enabled: false }
            Action { text: tx("action.form_dropdown"); enabled: false }
            MenuSeparator {}
            Action { text: tx("action.feature_planned"); enabled: false }
        }

        Menu {
            title: tx("menu.protect")
            Action { text: tx("action.password_permissions"); onTriggered: { if (requireProvider("qpdf")) securityDlg.open() } }
            Action { text: tx("action.decrypt"); enabled: pdfDocument.filePath !== ""; onTriggered: { if (requireProvider("qpdf")) decryptDlg.open() } }
            Action { text: tx("action.redact"); enabled: !pdfDocument.locked; onTriggered: chooseTool("redact") }
            Action { text: tx("action.safe_flatten"); enabled: pdfDocument.filePath !== ""; onTriggered: { toolOutputDlg.operation = "safeFlatten"; toolOutputDlg.open() } }
            Action { text: tx("action.lock_session"); onTriggered: pdfDocument.setLocked(!pdfDocument.locked) }
            MenuSeparator {}
            Action { text: tx("action.digital_signature"); enabled: false }
        }

        Menu {
            title: tx("menu.convert")
            Action { text: tx("action.export_images"); enabled: pdfDocument.filePath !== ""; onTriggered: exportImagesDlg.open() }
            Action { text: tx("action.image_to_pdf"); onTriggered: imageToPdfInputDlg.open() }
            Action { text: tx("action.office_to_pdf"); onTriggered: { if (requireProvider("libreOffice")) officeInputDlg.open() } }
            Action { text: tx("action.ocr"); enabled: pdfDocument.filePath !== ""; onTriggered: { if (requireProvider("qpdf") && requireProvider("tesseract")) ocrDlg.open() } }
        }

        Menu {
            title: tx("menu.tools")
            Action { text: tx("action.command_palette"); onTriggered: commandDlg.open() }
            Action { text: tx("action.providers"); onTriggered: providersDlg.open() }
            Action { text: tx("action.compare"); enabled: pdfDocument.filePath !== ""; onTriggered: compareDlg.open() }
            Action { text: tx("action.check"); enabled: pdfDocument.filePath !== ""; onTriggered: pdfTools.startCheckPdf(pdfDocument.filePath) }
            Action { text: tx("action.batch_optimize"); enabled: !pdfTools.busy; onTriggered: { if (requireProvider("qpdf")) batchOptimizeFilesDlg.open() } }
        }

        Menu {
            title: tx("menu.window")
            Action { text: tx("action.next_tab"); enabled: documentManager.count > 1; onTriggered: documentManager.currentIndex = (documentManager.currentIndex + 1) % documentManager.count }
            Action { text: tx("action.previous_tab"); enabled: documentManager.count > 1; onTriggered: documentManager.currentIndex = (documentManager.currentIndex - 1 + documentManager.count) % documentManager.count }
            Action { text: tx("action.close_tab"); onTriggered: requestCloseTab(documentManager.currentIndex) }
            Action { text: tx("action.close_other_tabs"); enabled: documentManager.count > 1 && !documentManager.hasModifiedDocuments; onTriggered: documentManager.closeOtherTabs(documentManager.currentIndex) }
        }

        Menu {
            title: tx("menu.language")
            Action { text: tx("language.english"); checkable: true; checked: i18n.language === "en"; onTriggered: i18n.language = "en" }
            Action { text: tx("language.arabic"); checkable: true; checked: i18n.language === "ar"; onTriggered: i18n.language = "ar" }
        }

        Menu {
            title: tx("menu.help")
            Action { text: tx("action.command_palette"); onTriggered: commandDlg.open() }
            Action { text: tx("action.keyboard_shortcuts"); onTriggered: shortcutsDlg.open() }
            Action { text: tx("action.check_updates"); onTriggered: appSettings.openReleasesPage() }
            Action { text: tx("action.project_page"); onTriggered: appSettings.openProjectPage() }
            Action { text: tx("action.privacy"); onTriggered: privacyDlg.open() }
            Action { text: tx("action.support"); enabled: appSettings.supportAvailable; onTriggered: appSettings.openSupportPage() }
            Action { text: tx("action.about"); onTriggered: aboutDlg.open() }
        }
    }

    header: Rectangle {
        height: 154
        color: panelColor
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: appSettings.darkMode ? "#18243a" : "#f0f6ff" }
            GradientStop { position: 0.5; color: appSettings.darkMode ? "#211f3a" : "#f8f5ff" }
            GradientStop { position: 1.0; color: appSettings.darkMode ? "#12333a" : "#eefcff" }
        }
        border.color: borderColor
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 4
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: accentColor }
                GradientStop { position: 0.25; color: accentViolet }
                GradientStop { position: 0.50; color: accentCyan }
                GradientStop { position: 0.75; color: accentEmerald }
                GradientStop { position: 1.0; color: accentAmber }
            }
            z: 5
        }
        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 58
                Layout.leftMargin: 18
                Layout.rightMargin: 18
                spacing: 10
                Rectangle {
                    width: 38
                    height: 38
                    radius: 11
                    color: accentColor
                    Label { anchors.centerIn: parent; text: "◈"; font.pixelSize: 25; font.bold: true; color: "white" }
                }
                Column {
                    Label { text: tx("app.brand"); font.bold: true; font.pixelSize: 16; color: textColor }
                    Label { text: tx("app.professional"); font.pixelSize: 9; font.letterSpacing: 1.5; color: mutedColor }
                }
                Rectangle { width: 1; height: 32; color: borderColor }
                ToolButton { text: tx("action.home"); onClicked: homeVisible = true }
                ToolButton { text: tx("action.new_tab"); onClicked: { documentManager.newTab(); homeVisible = false } }
                ToolButton { text: tx("action.open"); onClicked: openDlg.open() }
                ToolButton { text: tx("action.save"); onClicked: pdfDocument.filePath === "" ? saveDlg.open() : pdfDocument.save() }
                ToolButton { text: tx("action.print"); enabled: printService.available; onClicked: printService.printDocument(pdfDocument, false) }
                ToolButton { text: tx("dialog.find"); onClicked: searchDlg.open() }
                Item { Layout.fillWidth: true }
                Label { text: displayDocumentTitle(); color: mutedColor; elide: Text.ElideMiddle; Layout.maximumWidth: 330 }
                Button { text: tx("action.protect"); onClicked: { if (requireProvider("qpdf")) securityDlg.open() } }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                color: appSettings.darkMode ? "#172338" : "#f5f8ff"
                border.color: borderColor
                ListView {
                    id: tabsView
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    orientation: ListView.Horizontal
                    spacing: 4
                    clip: true
                    model: documentManager.tabs
                    delegate: Rectangle {
                        required property var modelData
                        width: Math.min(230, Math.max(130, tabRow.implicitWidth + 20))
                        height: 36
                        radius: 7
                        color: modelData.index === documentManager.currentIndex ? panelColor : "transparent"
                        border.color: modelData.index === documentManager.currentIndex ? borderColor : "transparent"
                        RowLayout {
                            id: tabRow
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 4
                            Label {
                                Layout.fillWidth: true
                                text: (modelData.modified ? "• " : "") + modelData.title
                                color: textColor
                                elide: Text.ElideMiddle
                            }
                            ToolButton { text: "×"; onClicked: requestCloseTab(modelData.index) }
                        }
                        TapHandler { onTapped: { documentManager.currentIndex = modelData.index; homeVisible = false } }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 54
                color: appSettings.darkMode ? "#132b31" : "#f2fbfc"
                border.color: borderColor
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 5
                    ToolButton { text: "↶"; enabled: pdfDocument.canUndo; onClicked: pdfDocument.undo() }
                    ToolButton { text: "↷"; enabled: pdfDocument.canRedo; onClicked: pdfDocument.redo() }
                    Rectangle { width: 1; height: 30; color: borderColor }

                    Button {
                        id: pointerToolButton
                        text: tx("action.pointer"); checkable: true; checked: tool === "select"; onClicked: chooseTool("select")
                        palette.buttonText: checked ? "white" : textColor
                        background: Rectangle { radius: 8; color: pointerToolButton.checked ? pointerColor : (pointerToolButton.hovered ? (appSettings.darkMode ? "#24364f" : "#e8f0ff") : "transparent"); border.color: pointerToolButton.checked ? pointerColor : borderColor }
                    }
                    Button {
                        id: handToolButton
                        text: tx("action.hand"); checkable: true; checked: tool === "hand"; onClicked: chooseTool("hand")
                        palette.buttonText: checked ? "white" : textColor
                        background: Rectangle { radius: 8; color: handToolButton.checked ? accentCyan : (handToolButton.hovered ? (appSettings.darkMode ? "#153d47" : "#e4f8fb") : "transparent"); border.color: handToolButton.checked ? accentCyan : borderColor }
                    }
                    Button {
                        id: textToolButton
                        text: tx("action.edit_text"); checkable: true; checked: tool === "text"; enabled: !pdfDocument.locked; onClicked: chooseTool("text")
                        palette.buttonText: checked ? "white" : textColor
                        background: Rectangle { radius: 8; color: textToolButton.checked ? textToolColor : (textToolButton.hovered ? (appSettings.darkMode ? "#33235c" : "#f1eafe") : "transparent"); border.color: textToolButton.checked ? textToolColor : borderColor }
                    }
                    Button {
                        id: highlightToolButton
                        text: tx("action.highlight"); checkable: true; checked: tool === "highlight"; enabled: !pdfDocument.locked; onClicked: chooseTool("highlight")
                        palette.buttonText: checked ? "white" : textColor
                        background: Rectangle { radius: 8; color: highlightToolButton.checked ? highlightToolColor : (highlightToolButton.hovered ? (appSettings.darkMode ? "#4b3719" : "#fff4d6") : "transparent"); border.color: highlightToolButton.checked ? highlightToolColor : borderColor }
                    }
                    Button {
                        id: drawToolButton
                        text: tx("action.draw"); checkable: true; checked: tool === "draw"; enabled: !pdfDocument.locked; onClicked: chooseTool("draw")
                        palette.buttonText: checked ? "white" : textColor
                        background: Rectangle { radius: 8; color: drawToolButton.checked ? drawToolColor : (drawToolButton.hovered ? (appSettings.darkMode ? "#153d47" : "#e4f8fb") : "transparent"); border.color: drawToolButton.checked ? drawToolColor : borderColor }
                    }
                    Button {
                        id: redactToolButton
                        text: tx("action.redact"); checkable: true; checked: tool === "redact"; enabled: !pdfDocument.locked; onClicked: chooseTool("redact")
                        palette.buttonText: checked ? "white" : textColor
                        background: Rectangle { radius: 8; color: redactToolButton.checked ? redactToolColor : (redactToolButton.hovered ? (appSettings.darkMode ? "#4a2025" : "#feecec") : "transparent"); border.color: redactToolButton.checked ? redactToolColor : borderColor }
                    }
                    Button {
                        id: cropToolButton
                        text: tx("action.crop"); checkable: true; checked: tool === "crop"; enabled: !pdfDocument.locked; onClicked: chooseTool("crop")
                        palette.buttonText: checked ? "white" : textColor
                        background: Rectangle { radius: 8; color: cropToolButton.checked ? cropToolColor : (cropToolButton.hovered ? (appSettings.darkMode ? "#173f34" : "#e7f8f1") : "transparent"); border.color: cropToolButton.checked ? cropToolColor : borderColor }
                    }

                    Rectangle { width: 1; height: 30; color: borderColor }
                    Label {
                        text: toolHint()
                        color: tool === "redact" ? redactToolColor : tool === "crop" ? cropToolColor : tool === "draw" ? drawToolColor : tool === "highlight" ? highlightToolColor : tool === "text" ? textToolColor : tool === "hand" ? accentCyan : pointerColor
                        font.pixelSize: 11
                        Layout.maximumWidth: 215
                        elide: Text.ElideRight
                    }
                    Button { visible: selectedText !== ""; text: tx("action.copy_text"); onClicked: pdfDocument.copyTextToClipboard(selectedText) }
                    Button { visible: selectedText !== "" && selectedTextPage >= 0; text: tx("action.highlight_selection"); onClicked: { pdfDocument.addHighlightRects(selectedTextPage, selectedTextRects, highlightColor.toString(), highlightOpacity); clearTextSelection() } }
                    Button { visible: selectedAnnotationId !== ""; text: tx("action.delete_selected_object"); onClicked: deleteCurrentSelection() }
                    Button { visible: pendingActionPage >= 0; text: tx("action.apply"); highlighted: true; onClicked: applyPendingAction() }
                    Button { visible: pendingActionPage >= 0; text: tx("dialog.cancel"); onClicked: cancelPendingAction() }

                    RowLayout {
                        visible: tool === "draw"
                        spacing: 4
                        Label { text: tx("tool.brush"); color: mutedColor; font.pixelSize: 10 }
                        Rectangle { width: 18; height: 18; radius: 9; color: "#2563eb"; border.color: drawColor.toString() === "#2563eb" ? "white" : borderColor; TapHandler { onTapped: drawColor = "#2563eb" } }
                        Rectangle { width: 18; height: 18; radius: 9; color: "#111827"; border.color: drawColor.toString() === "#111827" ? "white" : borderColor; TapHandler { onTapped: drawColor = "#111827" } }
                        Rectangle { width: 18; height: 18; radius: 9; color: "#dc2626"; border.color: drawColor.toString() === "#dc2626" ? "white" : borderColor; TapHandler { onTapped: drawColor = "#dc2626" } }
                        Slider { from: 1; to: 10; value: drawWidth; Layout.preferredWidth: 82; onMoved: drawWidth = value }
                    }
                    Item { Layout.fillWidth: true }
                    ToolButton { text: tx("action.fit_width"); onClicked: fitWidth() }
                    ToolButton { text: "−"; onClicked: zoomOut() }
                    Label { text: i18n.number(Math.round(zoom * 100)) + "%"; Layout.preferredWidth: 54; horizontalAlignment: Text.AlignHCenter; color: textColor }
                    ToolButton { text: "+"; onClicked: zoomIn() }
                }
            }
        }
    }

    footer: Rectangle {
        height: 32
        color: panelColor
        border.color: borderColor
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            Label { text: i18n.number(pdfDocument.currentPage + 1) + " / " + i18n.number(pdfDocument.pageCount); color: textColor }
            Label { text: pdfDocument.locked ? tx("status.editing_locked") : tx("status.ready"); color: pdfDocument.locked ? "#f59e0b" : mutedColor }
            Label {
                text: tx("status.performance_profile", [performanceProfileLabel()])
                color: appSettings.performanceProfile === "eco" ? accentEmerald : appSettings.performanceProfile === "performance" ? accentViolet : accentColor
            }
            Item { Layout.fillWidth: true }
            Label { text: pdfDocument.modified ? tx("status.unsaved") : tx("status.saved"); color: mutedColor }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: homeVisible ? 0 : 1

        Rectangle {
            color: appSettings.darkMode ? "#111827" : "#f4f6f9"
            ScrollView {
                anchors.fill: parent
                contentWidth: availableWidth
                ColumnLayout {
                    width: Math.min(980, parent.width - 40)
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 42
                    spacing: 18
                    Label { text: tx("home.title"); font.pixelSize: 32; font.bold: true; color: textColor; Layout.alignment: Qt.AlignHCenter }
                    Label { text: tx("home.subtitle"); font.pixelSize: 15; color: mutedColor; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 12
                        Button { text: tx("home.open"); highlighted: true; onClicked: openDlg.open() }
                        Button { text: tx("home.new"); onClicked: { documentManager.newTab(); homeVisible = false } }
                        Button { text: tx("home.image_pdf"); onClicked: imageToPdfInputDlg.open() }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: borderColor }
                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: tx("home.recent"); font.pixelSize: 18; font.bold: true; color: textColor }
                        Item { Layout.fillWidth: true }
                        Button { text: tx("home.clear_recent"); enabled: appSettings.recentFiles.length > 0; onClicked: appSettings.clearRecentFiles() }
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(340, Math.max(80, contentHeight))
                        model: appSettings.recentFiles
                        clip: true
                        spacing: 4
                        delegate: ItemDelegate {
                            required property var modelData
                            width: ListView.view.width
                            enabled: modelData.exists
                            contentItem: Column {
                                spacing: 2
                                Label { text: modelData.name; color: textColor; font.bold: true; elide: Text.ElideMiddle; width: parent.width }
                                Label { text: modelData.path; color: mutedColor; font.pixelSize: 11; elide: Text.ElideMiddle; width: parent.width }
                            }
                            onClicked: {
                                documentManager.openDocument(modelData.path)
                                homeVisible = false
                            }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 130
                        radius: 12
                        color: panelColor
                        border.color: borderColor
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 18
                            ColumnLayout {
                                Layout.fillWidth: true
                                Label { text: tx("home.privacy_heading"); font.bold: true; font.pixelSize: 17; color: textColor }
                                Label { text: tx("home.privacy_body"); wrapMode: Text.WordWrap; color: mutedColor; Layout.fillWidth: true }
                            }
                            Button { text: tx("action.privacy"); onClicked: privacyDlg.open() }
                        }
                    }
                }
            }
        }

        SplitView {
            orientation: Qt.Horizontal

            Rectangle {
                visible: showSidebar
                SplitView.preferredWidth: showSidebar ? 238 : 0
                SplitView.minimumWidth: showSidebar ? 170 : 0
                color: subPanelColor
                border.color: borderColor
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 12
                        Label { text: tx("section.pages"); font.bold: true; color: mutedColor }
                        Item { Layout.fillWidth: true }
                        ToolButton { text: "＋"; onClicked: pdfDocument.addBlankPage() }
                    }
                    ListView {
                        id: thumbs
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: pdfDocument.pages
                        clip: true
                        // Do not bind ListView.currentIndex to the document page:
                        // Qt may auto-scroll the thumbnail strip whenever the visible
                        // page changes. Highlighting is done in the delegate instead.
                        delegate: Item {
                            required property int index
                            required property string pageImage
                            width: thumbs.width
                            height: 190
                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 5
                                color: index === pdfDocument.currentPage ? (appSettings.darkMode ? "#1e3a5f" : "#e7efff") : "transparent"
                                radius: 8
                                border.color: index === pdfDocument.currentPage ? "#91aff0" : "transparent"
                            }
                            Image { source: pageImage; anchors.horizontalCenter: parent.horizontalCenter; y: 12; width: 104; height: 147; sourceSize.width: 104; sourceSize.height: 147; fillMode: Image.PreserveAspectFit; cache: false; asynchronous: true }
                            Label { text: i18n.number(index + 1); color: textColor; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 6 }
                            TapHandler { onTapped: navigateToPage(index) }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 7
                        Button { text: tx("action.copy"); onClicked: pdfDocument.copyPage(pdfDocument.currentPage) }
                        Button { text: tx("action.paste"); enabled: pdfDocument.hasPageClipboard; onClicked: pdfDocument.pastePage(pdfDocument.currentPage) }
                        Button { text: tx("action.delete_page"); enabled: pdfDocument.pageCount > 1; onClicked: pdfDocument.deletePage(pdfDocument.currentPage) }
                    }
                }
            }

            Rectangle {
                SplitView.fillWidth: true
                color: canvasColor
                gradient: Gradient {
                    GradientStop { position: 0.0; color: appSettings.darkMode ? "#0b1020" : "#e8edf5" }
                    GradientStop { position: 1.0; color: appSettings.darkMode ? "#111827" : "#d7deea" }
                }

                ListView {
                    id: documentView
                    anchors.fill: parent
                    orientation: ListView.Vertical
                    flickableDirection: Flickable.AutoFlickIfNeeded
                    spacing: 18
                    clip: true
                    reuseItems: true
                    cacheBuffer: appSettings.performanceProfile === "eco" ? Math.round(height * 0.55)
                                 : appSettings.performanceProfile === "performance" ? Math.round(height * 2.0)
                                 : Math.round(height * 1.25)
                    model: pdfDocument.pages
                    // currentIndex is intentionally not bound to currentPage. A
                    // binding here makes ListView try to reposition itself when
                    // Pointer/Text/Draw simply changes the active page.
                    highlightFollowsCurrentItem: false
                    boundsBehavior: Flickable.StopAtBounds
                    flickDeceleration: 2200
                    maximumFlickVelocity: 4200
                    contentWidth: Math.max(width, pdfDocument.pages.maxPageWidth() * zoom + 100)
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                    onContentYChanged: {
                        if (!moving && !flicking && count <= 0)
                            return
                        var probeX = contentX + Math.max(1, width / 2)
                        var probeY = contentY + Math.max(1, height / 2)
                        var idx = indexAt(probeX, probeY)
                        if (idx < 0)
                            idx = indexAt(contentX + 20, contentY + 20)
                        if (idx >= 0 && idx !== pdfDocument.currentPage) {
                            syncingPageFromScroll = true
                            pdfDocument.currentPage = idx
                            Qt.callLater(function() { syncingPageFromScroll = false })
                        }
                    }

                    delegate: Item {
                        id: pageDelegate
                        required property int index
                        required property string pageImage
                        required property real pageWidth
                        required property real pageHeight
                        width: documentView.contentWidth
                        height: Math.max(80, pageHeight * zoom + 28)
                        property real pressX: 0
                        property real pressY: 0
                        property real handStartContentX: 0
                        property real handStartContentY: 0
                        property var localInkPoints: []
                        property bool drawingInk: false
                        property int paintedInkPoints: 0
                        property bool clearInkCanvas: false

                        function normalizedRect(x1, y1, x2, y2) {
                            var left = Math.max(0, Math.min(x1, x2) / Math.max(1, pageSurface.width))
                            var top = Math.max(0, Math.min(y1, y2) / Math.max(1, pageSurface.height))
                            var right = Math.min(1, Math.max(x1, x2) / Math.max(1, pageSurface.width))
                            var bottom = Math.min(1, Math.max(y1, y2) / Math.max(1, pageSurface.height))
                            return ({"x": left, "y": top, "w": Math.max(0, right - left), "h": Math.max(0, bottom - top)})
                        }

                        function finishTextDrag(mouse, makeHighlight) {
                            var distance = Math.abs(mouse.x - pressX) + Math.abs(mouse.y - pressY)
                            if (distance < 6) {
                                if (!makeHighlight)
                                    clearTextSelection()
                                return
                            }
                            var result = pdfDocument.textSelection(index,
                                                                   pressX / Math.max(1, pageSurface.width),
                                                                   pressY / Math.max(1, pageSurface.height),
                                                                   mouse.x / Math.max(1, pageSurface.width),
                                                                   mouse.y / Math.max(1, pageSurface.height))
                            if (!result.valid || !result.text || result.rects.length === 0) {
                                if (makeHighlight) {
                                    var fallbackRect = pageDelegate.normalizedRect(pressX, pressY, mouse.x, mouse.y)
                                    if (fallbackRect.w >= 0.005 && fallbackRect.h >= 0.005)
                                        pdfDocument.addHighlightRects(index, [fallbackRect], highlightColor.toString(), highlightOpacity)
                                } else {
                                    clearTextSelection()
                                }
                                return
                            }
                            if (makeHighlight) {
                                pdfDocument.addHighlightRects(index, result.rects, highlightColor.toString(), highlightOpacity)
                                clearTextSelection()
                            } else {
                                selectedText = result.text
                                selectedTextRects = result.rects
                                selectedTextPage = index
                            }
                        }

                        Rectangle {
                            id: pageShadow
                            anchors.horizontalCenter: parent.horizontalCenter
                            y: 8
                            width: pageSurface.width + 10
                            height: pageSurface.height + 10
                            radius: 6
                            color: appSettings.darkMode ? "#50000000" : "#24000000"
                        }

                        Rectangle {
                            id: pageSurface
                            anchors.horizontalCenter: parent.horizontalCenter
                            y: 4
                            width: Math.max(40, pageWidth * zoom)
                            height: Math.max(50, pageHeight * zoom)
                            color: "white"
                            border.width: index === pdfDocument.currentPage ? 2 : 1
                            border.color: index === pdfDocument.currentPage ? accentColor : (appSettings.darkMode ? "#334155" : "#cbd3df")
                            radius: 2
                            clip: true

                            Image {
                                id: pageImg
                                anchors.fill: parent
                                fillMode: Image.Stretch
                                cache: false
                                asynchronous: true
                                sourceSize.width: Math.max(64, Math.round(pageSurface.width))
                                sourceSize.height: Math.max(64, Math.round(pageSurface.height))
                                source: {
                                    var modelRev = pdfDocument.pages.modelRevision
                                    return pageImage
                                }
                            }

                            BusyIndicator {
                                anchors.centerIn: parent
                                running: pageImg.status === Image.Loading
                                visible: running
                            }

                            Repeater {
                                model: selectedTextPage === index ? selectedTextRects : []
                                delegate: Rectangle {
                                    required property var modelData
                                    x: modelData.x * pageSurface.width
                                    y: modelData.y * pageSurface.height
                                    width: modelData.w * pageSurface.width
                                    height: modelData.h * pageSurface.height
                                    color: "#553b82f6"
                                    border.color: "#7aa8ff"
                                    border.width: 1
                                }
                            }

                            Repeater {
                                model: {
                                    var revisionDependency = pdfDocument.pages.modelRevision
                                    return pdfDocument.textAnnotations(index)
                                }
                                delegate: Rectangle {
                                    required property var modelData
                                    z: 28
                                    x: modelData.x * pageSurface.width
                                    y: modelData.y * pageSurface.height
                                    width: Math.max(18, modelData.w * pageSurface.width)
                                    height: Math.max(14, modelData.h * pageSurface.height)
                                    color: selectedAnnotationPage === pageDelegate.index && selectedAnnotationId === modelData.id ? "#223b82f6" : "transparent"
                                    border.width: selectedAnnotationPage === pageDelegate.index && selectedAnnotationId === modelData.id ? 2 : 0
                                    border.color: accentColor
                                    visible: tool === "select"
                                    TapHandler {
                                        enabled: tool === "select"
                                        onTapped: {
                                            selectedAnnotationPage = pageDelegate.index
                                            selectedAnnotationId = modelData.id
                                            clearTextSelection()
                                            pdfDocument.currentPage = pageDelegate.index
                                        }
                                    }
                                }
                            }

                            Canvas {
                                id: liveInk
                                anchors.fill: parent
                                visible: pageDelegate.drawingInk && tool === "draw"
                                renderStrategy: Canvas.Threaded
                                onPaint: {
                                    var ctx = getContext("2d")
                                    if (pageDelegate.clearInkCanvas) {
                                        ctx.clearRect(0, 0, width, height)
                                        pageDelegate.clearInkCanvas = false
                                        pageDelegate.paintedInkPoints = 0
                                    }
                                    var pts = pageDelegate.localInkPoints
                                    if (pts.length < 4)
                                        return
                                    var start = Math.max(0, pageDelegate.paintedInkPoints - 2)
                                    if (start + 3 >= pts.length)
                                        return
                                    ctx.beginPath()
                                    ctx.strokeStyle = drawColor.toString()
                                    ctx.globalAlpha = drawOpacity / 100.0
                                    ctx.lineWidth = drawWidth
                                    ctx.lineCap = "round"
                                    ctx.lineJoin = "round"
                                    ctx.moveTo(pts[start] * width, pts[start + 1] * height)
                                    for (var i = start + 2; i + 1 < pts.length; i += 2)
                                        ctx.lineTo(pts[i] * width, pts[i + 1] * height)
                                    ctx.stroke()
                                    pageDelegate.paintedInkPoints = pts.length
                                }
                            }

                            Rectangle {
                                id: dragPreview
                                z: 15
                                visible: interactionArea.pressed && (tool === "select" || tool === "highlight" || tool === "redact" || tool === "crop")
                                x: Math.min(pageDelegate.pressX, interactionArea.mouseX)
                                y: Math.min(pageDelegate.pressY, interactionArea.mouseY)
                                width: Math.abs(interactionArea.mouseX - pageDelegate.pressX)
                                height: Math.abs(interactionArea.mouseY - pageDelegate.pressY)
                                color: tool === "redact" ? "#33dc2626" : tool === "crop" ? "#22059669" : tool === "highlight" ? "#33f59e0b" : "#263b82f6"
                                border.width: 2
                                border.color: tool === "redact" ? redactToolColor : tool === "crop" ? cropToolColor : tool === "highlight" ? highlightToolColor : pointerColor
                            }

                            Rectangle {
                                id: pendingOverlay
                                z: 30
                                visible: pendingActionPage === index
                                x: pendingActionRect.x * pageSurface.width
                                y: pendingActionRect.y * pageSurface.height
                                width: pendingActionRect.w * pageSurface.width
                                height: pendingActionRect.h * pageSurface.height
                                color: pendingActionTool === "redact" ? "#44dc2626" : "#22059669"
                                border.width: 2
                                border.color: pendingActionTool === "redact" ? redactToolColor : cropToolColor

                                Repeater {
                                    model: pendingActionTool === "crop" ? 4 : 0
                                    delegate: Rectangle {
                                        required property int index
                                        width: 12
                                        height: 12
                                        radius: 6
                                        color: "white"
                                        border.color: cropToolColor
                                        border.width: 2
                                        x: (index === 0 || index === 2) ? -width / 2 : pendingOverlay.width - width / 2
                                        y: (index === 0 || index === 1) ? -height / 2 : pendingOverlay.height - height / 2
                                        MouseArea {
                                            anchors.fill: parent
                                            preventStealing: true
                                            cursorShape: Qt.SizeFDiagCursor
                                            onPositionChanged: function(mouse) {
                                                if (!pressed)
                                                    return
                                                var p = parent.mapToItem(pageSurface, mouse.x, mouse.y)
                                                var nx = Math.max(0, Math.min(1, p.x / Math.max(1, pageSurface.width)))
                                                var ny = Math.max(0, Math.min(1, p.y / Math.max(1, pageSurface.height)))
                                                var r = pendingActionRect
                                                var x2 = r.x + r.w
                                                var y2 = r.y + r.h
                                                if (index === 0) pendingActionRect = ({"x": Math.min(nx, x2 - 0.03), "y": Math.min(ny, y2 - 0.03), "w": Math.max(0.03, x2 - nx), "h": Math.max(0.03, y2 - ny)})
                                                else if (index === 1) pendingActionRect = ({"x": r.x, "y": Math.min(ny, y2 - 0.03), "w": Math.max(0.03, nx - r.x), "h": Math.max(0.03, y2 - ny)})
                                                else if (index === 2) pendingActionRect = ({"x": Math.min(nx, x2 - 0.03), "y": r.y, "w": Math.max(0.03, x2 - nx), "h": Math.max(0.03, ny - r.y)})
                                                else pendingActionRect = ({"x": r.x, "y": r.y, "w": Math.max(0.03, nx - r.x), "h": Math.max(0.03, ny - r.y)})
                                            }
                                        }
                                    }
                                }
                            }

                            MouseArea {
                                id: interactionArea
                                z: 20
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton
                                enabled: pendingActionPage < 0 && (tool === "select" || tool === "hand" || !pdfDocument.locked) && !(Qt.platform.os === "android" && tool === "select")
                                hoverEnabled: true
                                preventStealing: true
                                cursorShape: tool === "hand" ? (pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                                           : tool === "text" ? Qt.IBeamCursor
                                           : tool === "draw" || tool === "redact" || tool === "crop" || tool === "highlight" ? Qt.CrossCursor
                                           : Qt.IBeamCursor

                                onPressed: function(mouse) {
                                    // Mark the page active without asking ListView to reposition.
                                    // This prevents the old Pointer-click jump.
                                    pdfDocument.currentPage = index
                                    pageDelegate.pressX = mouse.x
                                    pageDelegate.pressY = mouse.y
                                    if (tool === "select")
                                        clearOverlaySelection()
                                    if (tool === "hand") {
                                        pageDelegate.handStartContentX = documentView.contentX
                                        pageDelegate.handStartContentY = documentView.contentY
                                    } else if (tool === "text") {
                                        textDlg.nx = mouse.x / Math.max(1, width)
                                        textDlg.ny = mouse.y / Math.max(1, height)
                                        textDlg.open()
                                    } else if (tool === "draw") {
                                        pageDelegate.localInkPoints = [mouse.x / Math.max(1, width), mouse.y / Math.max(1, height)]
                                        pageDelegate.paintedInkPoints = 0
                                        pageDelegate.clearInkCanvas = true
                                        pageDelegate.drawingInk = true
                                        liveInk.requestPaint()
                                    }
                                }

                                onPositionChanged: function(mouse) {
                                    if (pressed && tool === "hand") {
                                        var dxPan = mouse.x - pageDelegate.pressX
                                        var dyPan = mouse.y - pageDelegate.pressY
                                        var maxX = Math.max(0, documentView.contentWidth - documentView.width)
                                        var maxY = Math.max(0, documentView.contentHeight - documentView.height)
                                        documentView.contentX = Math.max(0, Math.min(maxX, pageDelegate.handStartContentX - dxPan))
                                        documentView.contentY = Math.max(0, Math.min(maxY, pageDelegate.handStartContentY - dyPan))
                                    } else if (pressed && tool === "draw" && pageDelegate.drawingInk) {
                                        var pts = pageDelegate.localInkPoints
                                        var nx = mouse.x / Math.max(1, width)
                                        var ny = mouse.y / Math.max(1, height)
                                        var count = pts.length
                                        var lx = count >= 2 ? pts[count - 2] * width : mouse.x
                                        var ly = count >= 2 ? pts[count - 1] * height : mouse.y
                                        var dx = mouse.x - lx
                                        var dy = mouse.y - ly
                                        if ((dx * dx + dy * dy) >= (minInkStepPx * minInkStepPx)) {
                                            pts.push(nx)
                                            pts.push(ny)
                                            liveInk.requestPaint()
                                        }
                                    }
                                }

                                onReleased: function(mouse) {
                                    if (tool === "hand") {
                                        return
                                    } else if (tool === "draw") {
                                        if (pageDelegate.localInkPoints.length >= 4)
                                            pdfDocument.addInkStyled(index, pageDelegate.localInkPoints, drawColor.toString(), Math.max(0.0006, drawWidth / Math.max(1, pageSurface.width)), drawOpacity)
                                        pageDelegate.localInkPoints = []
                                        pageDelegate.drawingInk = false
                                        pageDelegate.paintedInkPoints = 0
                                        pageDelegate.clearInkCanvas = true
                                        liveInk.requestPaint()
                                    } else if (tool === "select") {
                                        pageDelegate.finishTextDrag(mouse, false)
                                    } else if (tool === "highlight") {
                                        pageDelegate.finishTextDrag(mouse, true)
                                    } else if (tool === "redact" || tool === "crop") {
                                        var r = pageDelegate.normalizedRect(pageDelegate.pressX, pageDelegate.pressY, mouse.x, mouse.y)
                                        if (r.w >= 0.01 && r.h >= 0.01) {
                                            pendingActionPage = index
                                            pendingActionTool = tool
                                            pendingActionRect = r
                                        }
                                    }
                                }

                                onWheel: function(wheel) {
                                    if ((wheel.modifiers & Qt.ControlModifier) !== 0) {
                                        if (wheel.angleDelta.y > 0 || wheel.pixelDelta.y > 0)
                                            zoomIn()
                                        else
                                            zoomOut()
                                        wheel.accepted = true
                                        return
                                    }
                                    var delta = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y / 2
                                    var maxY = Math.max(0, documentView.contentHeight - documentView.height)
                                    documentView.contentY = Math.max(0, Math.min(maxY, documentView.contentY - delta))
                                    wheel.accepted = true
                                }

                                onCanceled: {
                                    pageDelegate.localInkPoints = []
                                    pageDelegate.drawingInk = false
                                    pageDelegate.paintedInkPoints = 0
                                    pageDelegate.clearInkCanvas = true
                                    liveInk.requestPaint()
                                }
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.margins: 8
                                width: pageBadge.implicitWidth + 14
                                height: 24
                                radius: 12
                                color: index === pdfDocument.currentPage ? accentColor : "#990f172a"
                                Label { id: pageBadge; anchors.centerIn: parent; text: i18n.number(index + 1); color: "white"; font.pixelSize: 10; font.bold: true }
                            }
                        }
                    }

                    footer: Item { width: documentView.width; height: 30 }
                }
            }

            Rectangle {
                SplitView.preferredWidth: 285
                SplitView.minimumWidth: 225
                color: panelColor
                border.color: borderColor
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 9
                    Label { text: tx("section.document"); font.bold: true; color: mutedColor }
                    Label { text: displayDocumentTitle(); Layout.fillWidth: true; elide: Text.ElideMiddle; font.pixelSize: 15; color: textColor }
                    Label { text: tx("label.pages", [i18n.number(pdfDocument.pageCount)]); color: mutedColor }
                    Rectangle { Layout.fillWidth: true; height: 1; color: borderColor }
                    Label { text: tx("section.quick_actions"); font.bold: true; color: mutedColor }
                    Button { text: tx("action.insert_image"); Layout.fillWidth: true; onClicked: imageDlg.open() }
                    Button { text: tx("action.signature"); Layout.fillWidth: true; onClicked: signatureDlg.open() }
                    Button { text: tx("action.add_watermark"); Layout.fillWidth: true; onClicked: waterDlg.open() }
                    Button { text: tx("action.add_page_numbers"); Layout.fillWidth: true; onClicked: pdfDocument.addPageNumbers() }
                    Button { text: tx("action.bates"); Layout.fillWidth: true; onClicked: batesDlg.open() }
                    Button { text: tx("action.combine_another"); Layout.fillWidth: true; onClicked: appendDlg.open() }
                    Rectangle { Layout.fillWidth: true; height: 1; color: borderColor }
                    Label { text: tx("section.security"); font.bold: true; color: mutedColor }
                    Switch { text: tx("action.lock_now"); checked: pdfDocument.locked; onToggled: pdfDocument.setLocked(checked) }
                    Button { text: tx("action.redact"); Layout.fillWidth: true; onClicked: chooseTool("redact") }
                    Button { text: tx("action.password_permissions"); enabled: providersState.qpdf; Layout.fillWidth: true; onClicked: securityDlg.open() }
                    Label { text: tx("security.explanation"); wrapMode: Text.WordWrap; Layout.fillWidth: true; color: mutedColor; font.pixelSize: 11 }
                    Item { Layout.fillHeight: true }
                    Button { text: tx("action.save_as_pdf"); Layout.fillWidth: true; highlighted: true; onClicked: saveDlg.open() }
                }
            }
        }
    }
}
