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
    property var inkPoints: []
    property real dragStartX: 0
    property real dragStartY: 0
    property bool homeVisible: true
    property bool allowWindowClose: false
    property int pendingCloseTab: -1
    property string pendingAfterSave: ""
    property string passwordPath: ""
    property var compareResults: []
    property string toolReport: ""
    property string pendingImagePath: ""
    property var providersState: pdfTools.providers

    readonly property color panelColor: appSettings.darkMode ? "#1f2937" : "#ffffff"
    readonly property color subPanelColor: appSettings.darkMode ? "#18212f" : "#f8f9fb"
    readonly property color canvasColor: appSettings.darkMode ? "#0b1020" : "#d9dde3"
    readonly property color textColor: appSettings.darkMode ? "#f3f4f6" : "#172033"
    readonly property color mutedColor: appSettings.darkMode ? "#aeb8c8" : "#667085"
    readonly property color borderColor: appSettings.darkMode ? "#334155" : "#dce1e8"
    readonly property color accentColor: "#2864dc"

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
        if (!pdfDocument)
            return
        var w = pdfDocument.pages.pageWidth(pdfDocument.currentPage)
        zoom = Math.max(0.12, Math.min(3.0, (view.width - 120) / Math.max(1, w)))
    }

    function fitPage() {
        if (!pdfDocument)
            return
        var w = pdfDocument.pages.pageWidth(pdfDocument.currentPage)
        var h = pdfDocument.pages.pageHeight(pdfDocument.currentPage)
        zoom = Math.max(0.12, Math.min(3.0, Math.min((view.width - 120) / Math.max(1, w), (view.height - 120) / Math.max(1, h))))
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
        onAccepted: {
            compareResults = pdfTools.comparePdf(pdfDocument.filePath, selectedFile, 0)
            compareResultDlg.open()
        }
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
                pdfTools.optimizePdf(pdfDocument.filePath, selectedFile)
            else if (operation === "linearize")
                pdfTools.linearizePdf(pdfDocument.filePath, selectedFile)
            else if (operation === "repair")
                pdfTools.repairPdf(pdfDocument.filePath, selectedFile)
            else if (operation === "safeFlatten")
                pdfTools.safeFlattenPdf(pdfDocument.filePath, selectedFile, 150)
            else if (operation === "ocr")
                pdfTools.ocrToSearchablePdf(pdfDocument.filePath, selectedFile, ocrLanguages.text, ocrDpi.value)
            else if (operation === "decrypt")
                pdfTools.decryptPdf(pdfDocument.filePath, selectedFile, decryptPassword.text)
        }
    }

    FolderDialog {
        id: imagesFolderDlg
        title: tx("file.choose_folder")
        onAccepted: pdfTools.exportImages(pdfDocument.filePath, selectedFolder, exportDpi.value)
    }

    FolderDialog {
        id: splitFolderDlg
        title: tx("file.choose_folder")
        onAccepted: pdfTools.splitPdf(pdfDocument.filePath, selectedFolder, splitPages.value)
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
        onAccepted: pdfTools.imageToPdf(pendingImagePath, selectedFile)
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
            pdfTools.officeToPdf(pendingOffice, selectedFolder)
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
            pdfDocument.addText(pdfDocument.currentPage, nx, ny, textInput.text, fontSize.value)
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
        id: commandDlg
        title: tx("dialog.command_title")
        modal: true
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            spacing: 6
            Button { text: tx("action.open"); Layout.fillWidth: true; onClicked: { commandDlg.close(); openDlg.open() } }
            Button { text: tx("action.find"); Layout.fillWidth: true; onClicked: { commandDlg.close(); searchDlg.open() } }
            Button { text: tx("action.compare"); Layout.fillWidth: true; enabled: pdfDocument.filePath !== ""; onClicked: { commandDlg.close(); compareDlg.open() } }
            Button { text: tx("action.ocr"); Layout.fillWidth: true; enabled: providersState.qpdf && providersState.tesseract && pdfDocument.filePath !== ""; onClicked: { commandDlg.close(); ocrDlg.open() } }
            Button { text: tx("action.safe_flatten"); Layout.fillWidth: true; enabled: pdfDocument.filePath !== ""; onClicked: { commandDlg.close(); toolOutputDlg.operation = "safeFlatten"; toolOutputDlg.open() } }
            Button { text: tx("action.privacy"); Layout.fillWidth: true; onClicked: { commandDlg.close(); privacyDlg.open() } }
        }
        footer: DialogButtonBox { Button { text: tx("dialog.close"); onClicked: commandDlg.close() } }
    }

    Shortcut { sequences: [StandardKey.New]; onActivated: { documentManager.newTab(); homeVisible = false } }
    Shortcut { sequences: [StandardKey.Open]; onActivated: openDlg.open() }
    Shortcut { sequences: [StandardKey.Save]; onActivated: pdfDocument.filePath === "" ? saveDlg.open() : pdfDocument.save() }
    Shortcut { sequences: [StandardKey.Find]; onActivated: searchDlg.open() }
    Shortcut { sequences: [StandardKey.Undo]; onActivated: if (pdfDocument.canUndo) pdfDocument.undo() }
    Shortcut { sequences: [StandardKey.Redo]; onActivated: if (pdfDocument.canRedo) pdfDocument.redo() }
    Shortcut { sequences: [StandardKey.Delete]; onActivated: pdfDocument.deletePage(pdfDocument.currentPage) }
    Shortcut { sequence: "Ctrl+K"; onActivated: commandDlg.open() }

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
            Action { text: tx("action.combine"); onTriggered: appendDlg.open() }
            MenuSeparator {}
            Action { text: tx("action.save"); onTriggered: pdfDocument.filePath === "" ? saveDlg.open() : pdfDocument.save() }
            Action { text: tx("action.save_as"); onTriggered: saveDlg.open() }
            Action { text: tx("action.extract"); onTriggered: extractDlg.open() }
            Action { text: tx("action.export_images"); enabled: pdfDocument.filePath !== ""; onTriggered: exportImagesDlg.open() }
        }
        Menu {
            title: tx("menu.edit")
            Action { text: tx("action.find"); onTriggered: searchDlg.open() }
            MenuSeparator {}
            Action { text: tx("action.undo"); enabled: pdfDocument.canUndo; onTriggered: pdfDocument.undo() }
            Action { text: tx("action.redo"); enabled: pdfDocument.canRedo; onTriggered: pdfDocument.redo() }
            MenuSeparator {}
            Action { text: tx("action.copy_page"); onTriggered: pdfDocument.copyPage(pdfDocument.currentPage) }
            Action { text: tx("action.paste_page"); enabled: pdfDocument.hasPageClipboard; onTriggered: pdfDocument.pastePage(pdfDocument.currentPage) }
        }
        Menu {
            title: tx("menu.pages")
            Action { text: tx("action.add_blank"); onTriggered: pdfDocument.addBlankPage() }
            Action { text: tx("action.duplicate"); onTriggered: pdfDocument.duplicatePage(pdfDocument.currentPage) }
            Action { text: tx("action.rotate_clockwise"); onTriggered: pdfDocument.rotatePage(pdfDocument.currentPage, 90) }
            Action { text: tx("action.crop"); onTriggered: { tool = "crop"; homeVisible = false } }
            Action { text: tx("action.bates"); onTriggered: batesDlg.open() }
            Action { text: tx("action.delete"); onTriggered: pdfDocument.deletePage(pdfDocument.currentPage) }
        }
        Menu {
            title: tx("menu.tools")
            Action { text: tx("action.compare"); enabled: pdfDocument.filePath !== ""; onTriggered: compareDlg.open() }
            Action { text: tx("action.ocr"); enabled: providersState.qpdf && providersState.tesseract && pdfDocument.filePath !== ""; onTriggered: ocrDlg.open() }
            Action { text: tx("action.optimize"); enabled: providersState.qpdf && pdfDocument.filePath !== ""; onTriggered: { toolOutputDlg.operation = "optimize"; toolOutputDlg.open() } }
            Action { text: tx("action.linearize"); enabled: providersState.qpdf && pdfDocument.filePath !== ""; onTriggered: { toolOutputDlg.operation = "linearize"; toolOutputDlg.open() } }
            Action { text: tx("action.repair"); enabled: providersState.qpdf && pdfDocument.filePath !== ""; onTriggered: { toolOutputDlg.operation = "repair"; toolOutputDlg.open() } }
            Action {
                text: tx("action.check")
                enabled: providersState.qpdf && pdfDocument.filePath !== ""
                onTriggered: {
                    toolReport = pdfTools.checkPdf(pdfDocument.filePath)
                    reportDlg.open()
                }
            }
            Action { text: tx("action.split"); enabled: providersState.qpdf && pdfDocument.filePath !== ""; onTriggered: splitDlg.open() }
            Action { text: tx("action.safe_flatten"); enabled: pdfDocument.filePath !== ""; onTriggered: { toolOutputDlg.operation = "safeFlatten"; toolOutputDlg.open() } }
            MenuSeparator {}
            Action { text: tx("action.image_to_pdf"); onTriggered: imageToPdfInputDlg.open() }
            Action { text: tx("action.office_to_pdf"); enabled: providersState.libreOffice; onTriggered: officeInputDlg.open() }
            Action { text: tx("action.providers"); onTriggered: providersDlg.open() }
        }
        Menu {
            title: tx("menu.protect")
            Action { text: tx("action.password_permissions"); enabled: providersState.qpdf; onTriggered: securityDlg.open() }
            Action { text: tx("action.decrypt"); enabled: providersState.qpdf && pdfDocument.filePath !== ""; onTriggered: decryptDlg.open() }
            Action { text: tx("action.redact"); onTriggered: { tool = "redact"; homeVisible = false } }
            Action { text: tx("action.lock_session"); onTriggered: pdfDocument.setLocked(!pdfDocument.locked) }
        }
        Menu {
            title: tx("menu.view")
            Action { text: tx("action.fit_width"); onTriggered: fitWidth() }
            Action { text: tx("action.fit_page"); onTriggered: fitPage() }
            Action { text: tx("action.dark_mode"); checkable: true; checked: appSettings.darkMode; onTriggered: appSettings.darkMode = checked }
            Action { text: tx("action.low_memory"); checkable: true; checked: appSettings.lowMemoryMode; onTriggered: appSettings.lowMemoryMode = checked }
        }
        Menu {
            title: tx("menu.language")
            Action { text: tx("language.english"); checkable: true; checked: i18n.language === "en"; onTriggered: i18n.language = "en" }
            Action { text: tx("language.arabic"); checkable: true; checked: i18n.language === "ar"; onTriggered: i18n.language = "ar" }
        }
        Menu {
            title: tx("menu.help")
            Action { text: tx("action.command_palette"); onTriggered: commandDlg.open() }
            Action { text: tx("action.privacy"); onTriggered: privacyDlg.open() }
            Action { text: tx("action.support"); enabled: appSettings.supportAvailable; onTriggered: appSettings.openSupportPage() }
            Action { text: tx("action.about"); onTriggered: aboutDlg.open() }
        }
    }

    header: Rectangle {
        height: 146
        color: panelColor
        border.color: borderColor
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
                ToolButton { text: tx("dialog.find"); onClicked: searchDlg.open() }
                Item { Layout.fillWidth: true }
                Label { text: displayDocumentTitle(); color: mutedColor; elide: Text.ElideMiddle; Layout.maximumWidth: 330 }
                Button { text: tx("action.protect"); enabled: providersState.qpdf; onClicked: securityDlg.open() }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                color: subPanelColor
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
                Layout.preferredHeight: 46
                color: subPanelColor
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    spacing: 5
                    ToolButton { text: "↶"; enabled: pdfDocument.canUndo; onClicked: pdfDocument.undo() }
                    ToolButton { text: "↷"; enabled: pdfDocument.canRedo; onClicked: pdfDocument.redo() }
                    Rectangle { width: 1; height: 28; color: borderColor }
                    Button { text: tx("action.pointer"); checkable: true; checked: tool === "select"; onClicked: tool = "select" }
                    Button { text: tx("action.edit_text"); checkable: true; checked: tool === "text"; enabled: !pdfDocument.locked; onClicked: tool = "text" }
                    Button { text: tx("action.highlight"); checkable: true; checked: tool === "highlight"; enabled: !pdfDocument.locked; onClicked: tool = "highlight" }
                    Button { text: tx("action.draw"); checkable: true; checked: tool === "draw"; enabled: !pdfDocument.locked; onClicked: tool = "draw" }
                    Button { text: tx("action.redact"); checkable: true; checked: tool === "redact"; enabled: !pdfDocument.locked; onClicked: tool = "redact" }
                    Button { text: tx("action.crop"); checkable: true; checked: tool === "crop"; enabled: !pdfDocument.locked; onClicked: tool = "crop" }
                    ToolButton { text: tx("action.signature"); enabled: !pdfDocument.locked; onClicked: signatureDlg.open() }
                    Item { Layout.fillWidth: true }
                    ToolButton { text: tx("action.fit_width"); onClicked: fitWidth() }
                    ToolButton { text: "−"; onClicked: zoom = Math.max(0.12, zoom - 0.1) }
                    Label { text: i18n.number(Math.round(zoom * 100)) + "%"; Layout.preferredWidth: 58; horizontalAlignment: Text.AlignHCenter; color: textColor }
                    ToolButton { text: "+"; onClicked: zoom = Math.min(3, zoom + 0.1) }
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
            Label { text: appSettings.lowMemoryMode ? tx("status.low_memory") : ""; color: mutedColor }
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
                SplitView.preferredWidth: 238
                SplitView.minimumWidth: 170
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
                        currentIndex: pdfDocument.currentPage
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
                            Image { source: pageImage; anchors.horizontalCenter: parent.horizontalCenter; y: 12; width: 104; height: 147; fillMode: Image.PreserveAspectFit; cache: false; asynchronous: false }
                            Label { text: i18n.number(index + 1); color: textColor; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 6 }
                            TapHandler { onTapped: pdfDocument.currentPage = index }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 7
                        Button { text: tx("action.copy"); onClicked: pdfDocument.copyPage(pdfDocument.currentPage) }
                        Button { text: tx("action.paste"); enabled: pdfDocument.hasPageClipboard; onClicked: pdfDocument.pastePage(pdfDocument.currentPage) }
                        Button { text: tx("action.delete"); enabled: pdfDocument.pageCount > 1; onClicked: pdfDocument.deletePage(pdfDocument.currentPage) }
                    }
                }
            }

            Rectangle {
                SplitView.fillWidth: true
                color: canvasColor
                Flickable {
                    id: view
                    anchors.fill: parent
                    contentWidth: page.width + 100
                    contentHeight: page.height + 100
                    clip: true
                    ScrollBar.vertical: ScrollBar {}
                    ScrollBar.horizontal: ScrollBar {}
                    Rectangle {
                        id: page
                        x: Math.max(50, (view.width - width) / 2)
                        y: 50
                        width: pdfDocument.pages.pageWidth(pdfDocument.currentPage) * zoom
                        height: pdfDocument.pages.pageHeight(pdfDocument.currentPage) * zoom
                        color: "white"
                        layer.enabled: true
                        Image {
                            id: pageImg
                            anchors.fill: parent
                            fillMode: Image.Stretch
                            cache: false
                            asynchronous: false
                            source: {
                                var modelRev = pdfDocument.pages.modelRevision
                                return pdfDocument.pages.imageSource(pdfDocument.currentPage)
                            }
                        }
                        Rectangle {
                            id: selectionRect
                            visible: interactionArea.pressed && (tool === "redact" || tool === "crop")
                            x: Math.min(dragStartX, interactionArea.mouseX)
                            y: Math.min(dragStartY, interactionArea.mouseY)
                            width: Math.abs(interactionArea.mouseX - dragStartX)
                            height: Math.abs(interactionArea.mouseY - dragStartY)
                            color: tool === "redact" ? "#80000000" : "#334f7cff"
                            border.width: 2
                            border.color: tool === "redact" ? "#ef4444" : "#2563eb"
                        }
                        MouseArea {
                            id: interactionArea
                            anchors.fill: parent
                            enabled: tool !== "select" && !pdfDocument.locked
                            hoverEnabled: true
                            onPressed: function(mouse) {
                                dragStartX = mouse.x
                                dragStartY = mouse.y
                                if (tool === "text") {
                                    textDlg.nx = mouse.x / width
                                    textDlg.ny = mouse.y / height
                                    textDlg.open()
                                } else if (tool === "highlight") {
                                    pdfDocument.addHighlight(pdfDocument.currentPage, mouse.x / width, mouse.y / height, 0.25, 0.035)
                                } else if (tool === "draw") {
                                    inkPoints = [mouse.x / width, mouse.y / height]
                                }
                            }
                            onPositionChanged: function(mouse) {
                                if (pressed && tool === "draw")
                                    inkPoints.push(mouse.x / width, mouse.y / height)
                            }
                            onReleased: function(mouse) {
                                if (tool === "draw") {
                                    pdfDocument.addInk(pdfDocument.currentPage, inkPoints)
                                    inkPoints = []
                                } else if (tool === "redact" || tool === "crop") {
                                    var x1 = dragStartX / width
                                    var y1 = dragStartY / height
                                    var x2 = mouse.x / width
                                    var y2 = mouse.y / height
                                    var nx = Math.min(x1, x2)
                                    var ny = Math.min(y1, y2)
                                    var nw = Math.abs(x2 - x1)
                                    var nh = Math.abs(y2 - y1)
                                    if (tool === "redact")
                                        pdfDocument.addRedaction(pdfDocument.currentPage, nx, ny, nw, nh)
                                    else
                                        pdfDocument.cropPage(pdfDocument.currentPage, nx, ny, nw, nh)
                                    tool = "select"
                                }
                            }
                        }
                    }
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
                    Button { text: tx("action.redact"); Layout.fillWidth: true; onClicked: tool = "redact" }
                    Button { text: tx("action.password_permissions"); enabled: providersState.qpdf; Layout.fillWidth: true; onClicked: securityDlg.open() }
                    Label { text: tx("security.explanation"); wrapMode: Text.WordWrap; Layout.fillWidth: true; color: mutedColor; font.pixelSize: 11 }
                    Item { Layout.fillHeight: true }
                    Button { text: tx("action.save_as_pdf"); Layout.fillWidth: true; highlighted: true; onClicked: saveDlg.open() }
                }
            }
        }
    }
}
