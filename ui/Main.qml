import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: win
    width: 1500
    height: 920
    visible: true
    color: "#eef1f5"
    title: (pdfDocument.modified ? "• " : "") + displayDocumentTitle() + " — " + tx("app.window_title")
    LayoutMirroring.enabled: i18n.rtl
    LayoutMirroring.childrenInherit: true

    property real zoom: 0.74
    property string tool: "select"
    property var inkPoints: []

    palette.window: "#f6f7f9"
    palette.windowText: "#172033"
    palette.button: "#ffffff"
    palette.buttonText: "#172033"
    palette.highlight: "#2864dc"

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

    Component.onCompleted: {
        if (pdfDocument.recoveryAvailable)
            recoveryDlg.open()
    }

    FileDialog {
        id: openDlg
        title: tx("file.open_pdf")
        nameFilters: [tx("file.pdf_filter")]
        onAccepted: pdfDocument.openDocument(selectedFile)
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
        onAccepted: pdfDocument.saveAs(selectedFile)
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
        id: protectDlg
        title: tx("file.create_protected")
        fileMode: FileDialog.SaveFile
        nameFilters: [tx("file.pdf_filter")]
        defaultSuffix: "pdf"
        onAccepted: pdfDocument.protectCopy(selectedFile, userPass.text, ownerPass.text,
                                             allowPrint.checked, allowCopy.checked, allowModify.checked)
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
            RowLayout { Label { text: tx("dialog.size") }; SpinBox { id: fontSize; from: 8; to: 96; value: 18 } }
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
            RowLayout { Label { text: tx("dialog.opacity") }; Slider { id: waterOpacity; from: 10; to: 90; value: 35; Layout.fillWidth: true } }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: waterDlg.reject() }
            Button { text: tx("dialog.ok"); highlighted: true; onClicked: waterDlg.accept() }
        }
        onAccepted: pdfDocument.addWatermark(waterText.text, 42, waterOpacity.value)
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
            Label { text: tx("dialog.security_provider"); color: "#667085" }
        }
        footer: DialogButtonBox {
            Button { text: tx("dialog.cancel"); onClicked: securityDlg.reject() }
            Button { text: tx("dialog.save"); highlighted: true; onClicked: securityDlg.accept() }
        }
        onAccepted: protectDlg.open()
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
            Label { text: tx("dialog.recovery_choice"); color: "#667085"; wrapMode: Text.WordWrap }
            RowLayout {
                Button { text: tx("dialog.recover"); highlighted: true; onClicked: { pdfDocument.recoverAutosave(); recoveryDlg.close() } }
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
            Label { text: tx("dialog.results", [i18n.number(searchDlg.hits.length)]); color: "#667085" }
            ListView {
                Layout.preferredWidth: 560
                Layout.preferredHeight: 360
                clip: true
                model: searchDlg.hits
                delegate: ItemDelegate {
                    required property var modelData
                    width: ListView.view.width
                    text: tx("dialog.page_result", [i18n.number(modelData.page + 1), modelData.excerpt])
                    onClicked: { pdfDocument.currentPage = modelData.page; searchDlg.close() }
                }
            }
            Label { text: tx("dialog.search_note"); color: "#667085"; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
        footer: DialogButtonBox { Button { text: tx("dialog.close"); onClicked: searchDlg.close() } }
    }

    Dialog {
        id: aboutDlg
        title: tx("dialog.about_title")
        standardButtons: Dialog.NoButton
        ColumnLayout {
            anchors.fill: parent
            Label { text: "◈"; font.pixelSize: 52; color: "#2864dc"; Layout.alignment: Qt.AlignHCenter }
            Label { text: tx("dialog.about_product"); font.pixelSize: 22; font.bold: true; Layout.alignment: Qt.AlignHCenter }
            Label { text: tx("dialog.about_body"); horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; Layout.preferredWidth: 460 }
        }
        footer: DialogButtonBox { Button { text: tx("dialog.ok"); onClicked: aboutDlg.close() } }
    }


    // Keyboard shortcuts remain active without exposing Latin shortcut labels in Arabic menus.
    Shortcut { sequences: [StandardKey.New]; onActivated: pdfDocument.newDocument() }
    Shortcut { sequences: [StandardKey.Open]; onActivated: openDlg.open() }
    Shortcut { sequences: [StandardKey.Save]; onActivated: pdfDocument.filePath === "" ? saveDlg.open() : pdfDocument.save() }
    Shortcut { sequences: [StandardKey.Find]; onActivated: searchDlg.open() }
    Shortcut { sequences: [StandardKey.Undo]; onActivated: if (pdfDocument.canUndo) pdfDocument.undo() }
    Shortcut { sequences: [StandardKey.Redo]; onActivated: if (pdfDocument.canRedo) pdfDocument.redo() }
    Shortcut { sequences: [StandardKey.Delete]; onActivated: pdfDocument.deletePage(pdfDocument.currentPage) }

    Connections {
        target: pdfDocument
        function onErrorOccurred(key, args) { toast.text = translatedMessage(key, args); toast.open() }
        function onInfo(key, args) { toast.text = translatedMessage(key, args); toast.open() }
    }

    Popup {
        id: toast
        x: (win.width - width) / 2
        y: win.height - height - 45
        padding: 14
        property string text: ""
        background: Rectangle { radius: 10; color: "#172033" }
        contentItem: Label { color: "white"; text: toast.text }
        Timer { running: toast.visible; interval: 2600; onTriggered: toast.close() }
    }

    menuBar: MenuBar {
        Menu {
            title: tx("menu.file")
            Action { text: tx("action.new"); onTriggered: pdfDocument.newDocument() }
            Action { text: tx("action.open"); onTriggered: openDlg.open() }
            Action { text: tx("action.combine"); onTriggered: appendDlg.open() }
            MenuSeparator {}
            Action { text: tx("action.save"); onTriggered: pdfDocument.filePath === "" ? saveDlg.open() : pdfDocument.save() }
            Action { text: tx("action.save_as"); onTriggered: saveDlg.open() }
            Action { text: tx("action.extract"); onTriggered: extractDlg.open() }
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
            Action { text: tx("action.delete"); onTriggered: pdfDocument.deletePage(pdfDocument.currentPage) }
        }
        Menu {
            title: tx("menu.protect")
            Action { text: tx("action.password_permissions"); onTriggered: securityDlg.open() }
            Action { text: tx("action.lock_session"); onTriggered: pdfDocument.setLocked(!pdfDocument.locked) }
        }
        Menu {
            title: tx("menu.language")
            Action { text: tx("language.english"); checkable: true; checked: i18n.language === "en"; onTriggered: i18n.language = "en" }
            Action { text: tx("language.arabic"); checkable: true; checked: i18n.language === "ar"; onTriggered: i18n.language = "ar" }
        }
        Menu {
            title: tx("menu.help")
            Action {
                text: tx("action.engine_capabilities")
                onTriggered: {
                    var c = pdfDocument.engineCapabilities()
                    toast.text = tx("capabilities.summary", [c.structuralTextEditing ? tx("common.yes") : tx("common.not_yet"), c.lazyRendering ? tx("common.yes") : tx("common.no")])
                    toast.open()
                }
            }
            Action { text: tx("action.about"); onTriggered: aboutDlg.open() }
        }
    }

    header: Rectangle {
        height: 112
        color: "#ffffff"
        border.color: "#dce1e8"
        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 58
                Layout.leftMargin: 18
                Layout.rightMargin: 18
                spacing: 10
                Rectangle { width: 38; height: 38; radius: 11; color: "#2864dc"; Label { anchors.centerIn: parent; text: "◈"; font.pixelSize: 25; font.bold: true; color: "white" } }
                Column {
                    Label { text: tx("app.brand"); font.bold: true; font.pixelSize: 16; color: "#172033" }
                    Label { text: tx("app.professional"); font.pixelSize: 9; letterSpacing: 1.5; color: "#667085" }
                }
                Rectangle { width: 1; height: 32; color: "#e1e5eb" }
                ToolButton { text: "＋ " + tx("action.new"); onClicked: pdfDocument.newDocument() }
                ToolButton { text: tx("action.open"); onClicked: openDlg.open() }
                ToolButton { text: tx("action.combine"); onClicked: appendDlg.open() }
                ToolButton { text: tx("action.save"); onClicked: pdfDocument.filePath === "" ? saveDlg.open() : pdfDocument.save() }
                ToolButton { text: tx("dialog.find"); onClicked: searchDlg.open() }
                Item { Layout.fillWidth: true }
                Label { text: displayDocumentTitle(); color: "#667085"; elide: Text.ElideMiddle; Layout.maximumWidth: 330 }
                Button { text: tx("action.protect"); onClicked: securityDlg.open() }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 54
                color: "#f8f9fb"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    spacing: 5
                    ToolButton { text: "↶"; enabled: pdfDocument.canUndo; onClicked: pdfDocument.undo() }
                    ToolButton { text: "↷"; enabled: pdfDocument.canRedo; onClicked: pdfDocument.redo() }
                    Rectangle { width: 1; height: 28; color: "#dce1e8" }
                    Button { text: tx("action.pointer"); checkable: true; checked: tool === "select"; onClicked: tool = "select" }
                    Button { text: tx("action.edit_text"); checkable: true; checked: tool === "text"; enabled: !pdfDocument.locked; onClicked: tool = "text" }
                    Button { text: tx("action.highlight"); checkable: true; checked: tool === "highlight"; enabled: !pdfDocument.locked; onClicked: tool = "highlight" }
                    Button { text: tx("action.draw"); checkable: true; checked: tool === "draw"; enabled: !pdfDocument.locked; onClicked: tool = "draw" }
                    ToolButton { text: tx("action.image"); enabled: !pdfDocument.locked; onClicked: imageDlg.open() }
                    ToolButton { text: tx("action.watermark"); enabled: !pdfDocument.locked; onClicked: waterDlg.open() }
                    ToolButton { text: tx("action.page_number"); enabled: !pdfDocument.locked; onClicked: pdfDocument.addPageNumbers() }
                    Rectangle { width: 1; height: 28; color: "#dce1e8" }
                    ToolButton { text: tx("action.rotate"); onClicked: pdfDocument.rotatePage(pdfDocument.currentPage, 90) }
                    ToolButton { text: tx("action.extract_short"); onClicked: extractDlg.open() }
                    Item { Layout.fillWidth: true }
                    ToolButton { text: "−"; onClicked: zoom = Math.max(0.25, zoom - 0.1) }
                    Label { text: i18n.number(Math.round(zoom * 100)) + "%"; Layout.preferredWidth: 58; horizontalAlignment: Text.AlignHCenter }
                    ToolButton { text: "+"; onClicked: zoom = Math.min(3, zoom + 0.1) }
                }
            }
        }
    }

    footer: Rectangle {
        height: 32
        color: "#fff"
        border.color: "#e1e5eb"
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            Label { text: i18n.number(pdfDocument.currentPage + 1) + " / " + i18n.number(pdfDocument.pageCount) }
            Label { text: pdfDocument.locked ? tx("status.editing_locked") : tx("status.ready"); color: pdfDocument.locked ? "#b54708" : "#667085" }
            Item { Layout.fillWidth: true }
            Label { text: pdfDocument.modified ? tx("status.unsaved") : tx("status.saved"); color: "#667085" }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Rectangle {
            SplitView.preferredWidth: 238
            SplitView.minimumWidth: 170
            color: "#f7f8fa"
            border.color: "#dce1e8"
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 12
                    Label { text: tx("section.pages"); font.bold: true; color: "#475467" }
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
                        Rectangle { anchors.fill: parent; anchors.margins: 5; color: index === pdfDocument.currentPage ? "#e7efff" : "transparent"; radius: 8; border.color: index === pdfDocument.currentPage ? "#91aff0" : "transparent" }
                        Image { source: pageImage; anchors.horizontalCenter: parent.horizontalCenter; y: 12; width: 104; height: 147; fillMode: Image.PreserveAspectFit; cache: false; asynchronous: false }
                        Label { text: i18n.number(index + 1); anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; bottomPadding: 6 }
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
            color: "#d9dde3"
            Flickable {
                id: view
                anchors.fill: parent
                contentWidth: page.width + 100
                contentHeight: page.height + 100
                clip: true
                Rectangle {
                    id: page
                    x: Math.max(50, (view.width - width) / 2)
                    y: 50
                    width: 595 * zoom
                    height: 842 * zoom
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
                    MouseArea {
                        anchors.fill: parent
                        enabled: tool !== "select" && !pdfDocument.locked
                        onPressed: function(mouse) {
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
                            if (pressed && tool === "draw") inkPoints.push(mouse.x / width, mouse.y / height)
                        }
                        onReleased: {
                            if (tool === "draw") {
                                pdfDocument.addInk(pdfDocument.currentPage, inkPoints)
                                inkPoints = []
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            SplitView.preferredWidth: 270
            SplitView.minimumWidth: 220
            color: "#fff"
            border.color: "#dce1e8"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10
                Label { text: tx("section.document"); font.bold: true; color: "#475467" }
                Label { text: displayDocumentTitle(); Layout.fillWidth: true; elide: Text.ElideMiddle; font.pixelSize: 15 }
                Label { text: tx("label.pages", [i18n.number(pdfDocument.pageCount)]); color: "#667085" }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#e5e7eb" }
                Label { text: tx("section.quick_actions"); font.bold: true; color: "#475467" }
                Button { text: tx("action.insert_image"); Layout.fillWidth: true; onClicked: imageDlg.open() }
                Button { text: tx("action.add_watermark"); Layout.fillWidth: true; onClicked: waterDlg.open() }
                Button { text: tx("action.add_page_numbers"); Layout.fillWidth: true; onClicked: pdfDocument.addPageNumbers() }
                Button { text: tx("action.combine_another"); Layout.fillWidth: true; onClicked: appendDlg.open() }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#e5e7eb" }
                Label { text: tx("section.security"); font.bold: true; color: "#475467" }
                Switch { text: tx("action.lock_now"); checked: pdfDocument.locked; onToggled: pdfDocument.setLocked(checked) }
                Button { text: tx("action.password_permissions"); Layout.fillWidth: true; onClicked: securityDlg.open() }
                Label { text: tx("security.explanation"); wrapMode: Text.WordWrap; Layout.fillWidth: true; color: "#667085"; font.pixelSize: 11 }
                Item { Layout.fillHeight: true }
                Button { text: tx("action.save_as_pdf"); Layout.fillWidth: true; highlighted: true; onClicked: saveDlg.open() }
            }
        }
    }
}
