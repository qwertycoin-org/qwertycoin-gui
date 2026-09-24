import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

Item {
    id: root
    property int contentHeight: showContactManagement ? 800 : 700
    property bool showContactManagement: false
    property string selectedFingerprint: ""
    property var contactsData: currentWallet ? currentWallet.messenger.contacts : []
    property var messagesData: currentWallet ? currentWallet.messenger.messages : []
    property var selectedContact: contactForFingerprint(contactsData, selectedFingerprint)
    property var filteredMessages: messagesForContact(messagesData, selectedFingerprint)

    function contactForFingerprint(contacts, fingerprint) {
        for (var i = 0; i < contacts.length; ++i)
            if (contacts[i].fingerprint === fingerprint) return contacts[i]
        return null
    }
    function messagesForContact(messages, fingerprint) {
        var result = []
        if (fingerprint === "") return result
        for (var i = 0; i < messages.length; ++i)
            if (messages[i].contact === fingerprint) result.push(messages[i])
        return result
    }
    function statusLabel(status) {
        if (status === "prepared") return qsTr("Ready to send")
        if (status === "broadcast" || status === "sent") return qsTr("Sent")
        if (status === "broadcast failed" || status === "send failed") return qsTr("Send failed")
        if (status === "confirmed") return qsTr("Confirmed")
        if (status === "chain proof lost") return qsTr("Confirmation lost")
        return status
    }
    function timestampLabel(timestamp) {
        if (!timestamp) return ""
        var value = new Date(timestamp)
        if (isNaN(value.getTime())) return ""
        return value.toLocaleString(Qt.locale(), "dd.MM.yyyy HH:mm")
    }
    onSelectedFingerprintChanged: {
        var contact = contactForFingerprint(contactsData, selectedFingerprint)
        renameField.text = contact ? contact.label : ""
        Qt.callLater(function() { chatList.positionViewAtEnd() })
    }

    Dialog {
        id: removeDialog
        width: 500
        modal: true
        title: qsTr("Remove contact?")
        standardButtons: Dialog.Yes | Dialog.No
        x: Math.max(0, (root.width - width) / 2)
        y: 160
        contentItem: Label {
            wrapMode: Text.WordWrap
            text: qsTr("Remove %1 from the contact list? The encrypted ratchet state is retained so delayed messages remain decryptable. Any locally retained chat history is unchanged.").arg(root.selectedContact ? root.selectedContact.label : "")
        }
        onAccepted: if (currentWallet && currentWallet.messenger.removeContact(root.selectedFingerprint)) root.selectedFingerprint = ""
    }

    ColumnLayout {
        id: page; width: parent.width; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Label { text: root.showContactManagement ? qsTr("Messenger contacts") : qsTr("Messenger"); font.pixelSize: 28; font.bold: true }
            Item { Layout.fillWidth: true }
            Button {
                objectName: "messengerContactManagementButton"
                text: root.showContactManagement ? qsTr("Back to chat") : qsTr("Manage contacts")
                onClicked: root.showContactManagement = !root.showContactManagement
            }
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: "#8a4b00"
            text: qsTr("Experimental QMS2 messenger: PQXDH plus an ongoing Triple Ratchet. The protocol has not received an independent security audit. Blockchain timing, fees and carrier count remain public. Use only test funds during the preview.")
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            visible: currentWallet && !currentWallet.messenger.ready
            color: "#d93025"
            text: currentWallet ? currentWallet.messenger.status : ""
        }

        GroupBox {
            visible: root.showContactManagement
            title: qsTr("My personal invitation"); Layout.fillWidth: true
            ColumnLayout { anchors.fill: parent
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; font.bold: true; text: qsTr("Send the long invitation below to your contact through a trusted private channel. Compare the shorter fingerprint separately before accepting the contact.") }
                Label { text: qsTr("Fingerprint (verify separately)") }
                TextField { Layout.fillWidth: true; readOnly: true; text: currentWallet ? currentWallet.messenger.ownFingerprint : ""; selectByMouse: true }
                Label { text: qsTr("Long personal invitation (send this to your contact)") }
                TextArea { Layout.fillWidth: true; Layout.preferredHeight: 92; readOnly: true; wrapMode: TextEdit.WrapAnywhere; text: currentWallet ? currentWallet.messenger.ownInvitation : ""; selectByMouse: true }
            }
        }

        GroupBox {
            visible: root.showContactManagement
            title: qsTr("Local message history")
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: qsTr("Message plaintext is not persisted by default. Enabling history stores it inside the password-bound encrypted QMS2 wallet state. This does not remove carrier data from the blockchain.")
                }
                RowLayout {
                    Layout.fillWidth: true
                    Switch {
                        text: qsTr("Persist decrypted chat history")
                        checked: currentWallet ? currentWallet.messenger.historyEnabled : false
                        enabled: currentWallet && currentWallet.messenger.ready
                        onToggled: if (currentWallet) currentWallet.messenger.setHistoryEnabled(checked)
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: qsTr("Clear local history")
                        enabled: currentWallet && currentWallet.messenger.ready
                        onClicked: currentWallet.messenger.clearHistory()
                    }
                }
            }
        }

        GroupBox {
            visible: root.showContactManagement
            title: qsTr("Add a messenger contact"); Layout.fillWidth: true
            ColumnLayout { anchors.fill: parent
                Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Paste the other wallet's long personal invitation. A payment address or fingerprint alone is not sufficient.") }
                TextField { id: contactLabel; Layout.fillWidth: true; placeholderText: qsTr("Contact name") }
                TextArea { id: contactInvitation; Layout.fillWidth: true; Layout.preferredHeight: 82; placeholderText: qsTr("Long invitation hex"); wrapMode: TextEdit.WrapAnywhere }
                Button {
                    text: qsTr("Import invitation")
                    enabled: currentWallet && currentWallet.messenger.ready && contactLabel.text.trim().length > 0 && contactInvitation.text.trim().length > 0
                    onClicked: if (currentWallet.messenger.importInvitation(contactLabel.text, contactInvitation.text)) { contactInvitation.clear(); contactLabel.clear() }
                }
            }
        }

        GroupBox {
            visible: root.showContactManagement && root.selectedContact !== null
            title: qsTr("Selected contact"); Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                Label { Layout.fillWidth: true; elide: Text.ElideMiddle; text: root.selectedFingerprint; color: "#667781" }
                RowLayout {
                    Layout.fillWidth: true
                    TextField { id: renameField; Layout.fillWidth: true; placeholderText: qsTr("Contact name") }
                    Button { text: qsTr("Rename"); enabled: renameField.text.trim().length > 0; onClicked: currentWallet.messenger.renameContact(root.selectedFingerprint, renameField.text) }
                    Button { text: qsTr("Remove"); enabled: !currentWallet || currentWallet.messenger.preparedTransactionCount === 0; onClicked: removeDialog.open() }
                }
            }
        }

        RowLayout {
            visible: !root.showContactManagement
            Layout.fillWidth: true; Layout.preferredHeight: 560; spacing: 12
            Rectangle {
                Layout.preferredWidth: 260; Layout.fillHeight: true; color: "#f0f2f5"; border.color: "#c8ccd0"; radius: 6
                ColumnLayout { anchors.fill: parent; anchors.margins: 8; spacing: 8
                    Label { text: qsTr("Contacts"); font.pixelSize: 18; font.bold: true; color: "#202c33" }
                    Label { Layout.fillWidth: true; visible: contactsData.length === 0; wrapMode: Text.WordWrap; color: "#667781"; text: qsTr("Import a personal invitation to start a conversation.") }
                    ListView {
                        id: contacts; objectName: "messengerContacts"; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 4; model: root.contactsData
                        delegate: Button {
                            width: contacts.width; height: 66; checkable: true
                            checked: root.selectedFingerprint === modelData.fingerprint
                            enabled: currentWallet && currentWallet.messenger.ready && (currentWallet.messenger.preparedTransactionCount === 0 || currentWallet.messenger.preparedContactFingerprint === modelData.fingerprint)
                            background: Rectangle { radius: 5; color: parent.checked ? "#d9fdd3" : (parent.hovered ? "#e7e9eb" : "transparent"); border.width: parent.checked ? 2 : 0; border.color: "#00a884" }
                            contentItem: Column { spacing: 3
                                Text { text: modelData.label; color: "#111b21"; font.bold: root.selectedFingerprint === modelData.fingerprint; font.pixelSize: 16; elide: Text.ElideRight; width: parent.width }
                                Text { text: modelData.fingerprint.substring(0, 20) + "…"; color: "#667781"; font.pixelSize: 12; elide: Text.ElideRight; width: parent.width }
                            }
                            onClicked: root.selectedFingerprint = modelData.fingerprint
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; color: "#efeae2"; border.color: "#c8ccd0"; radius: 6
                ColumnLayout { anchors.fill: parent; spacing: 0
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 68; color: "#f0f2f5"; visible: root.selectedContact !== null
                        RowLayout { anchors.fill: parent; anchors.margins: 10
                            ColumnLayout { Layout.fillWidth: true
                                Label { text: root.selectedContact ? root.selectedContact.label : ""; color: "#111b21"; font.pixelSize: 18; font.bold: true }
                                Label { text: root.selectedFingerprint; color: "#667781"; font.pixelSize: 11; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            }
                        }
                    }
                    Label { Layout.fillWidth: true; Layout.fillHeight: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: "#667781"; font.pixelSize: 18; visible: root.selectedContact === null; text: qsTr("Select a contact to open the conversation") }
                    ListView {
                        id: chatList; objectName: "messengerChat"; Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 12; clip: true; spacing: 4
                        visible: root.selectedContact !== null; model: root.filteredMessages
                        onCountChanged: Qt.callLater(function() { chatList.positionViewAtEnd() })
                        delegate: Item {
                            width: chatList.width; height: bubble.height + 8
                            property bool mine: modelData.direction === "out"
                            Rectangle {
                                id: bubble
                                width: Math.min(chatList.width * 0.72, Math.max(190, messageText.implicitWidth + 28))
                                height: messageColumn.implicitHeight + 20
                                anchors.right: parent.mine ? parent.right : undefined
                                anchors.left: parent.mine ? undefined : parent.left
                                color: parent.mine ? "#d9fdd3" : "#ffffff"; radius: 8
                                Column { id: messageColumn; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 10; spacing: 4
                                    Text { width: parent.width; text: bubble.parent.mine ? qsTr("Me") : (root.selectedContact ? root.selectedContact.label : modelData.label); color: bubble.parent.mine ? "#008069" : "#027eb5"; font.bold: true; font.pixelSize: 12 }
                                    Text { id: messageText; width: parent.width; text: modelData.text; color: "#111b21"; wrapMode: Text.Wrap; font.pixelSize: 15 }
                                    Text {
                                        width: parent.width; horizontalAlignment: Text.AlignRight; color: "#667781"; font.pixelSize: 10
                                        text: {
                                            var time = root.timestampLabel(modelData.timestamp)
                                            return time === "" ? root.statusLabel(modelData.status) : time + " · " + root.statusLabel(modelData.status)
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: composerColumn.implicitHeight + 20; color: "#f0f2f5"; visible: root.selectedContact !== null
                        ColumnLayout { id: composerColumn; anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.margins: 10; spacing: 6
                            TextArea { id: composer; objectName: "messengerComposer"; Layout.fillWidth: true; Layout.preferredHeight: 92; enabled: currentWallet && currentWallet.messenger.ready && currentWallet.messenger.preparedTransactionCount === 0; placeholderText: qsTr("Write an encrypted message (maximum 4,096 UTF-8 bytes)"); wrapMode: TextEdit.Wrap }
                            RowLayout { Layout.fillWidth: true
                                Label { text: qsTr("UTF-8 bytes: %1 / 4096").arg(unescape(encodeURIComponent(composer.text)).length); color: unescape(encodeURIComponent(composer.text)).length > 4096 ? "#d93025" : "#667781" }
                                Item { Layout.fillWidth: true }
                                Button {
                                    text: qsTr("Encrypt & review")
                                    enabled: root.selectedFingerprint !== "" && composer.text.length > 0 && unescape(encodeURIComponent(composer.text)).length <= 4096 && currentWallet.messenger.preparedTransactionCount === 0
                                    onClicked: if (currentWallet.messenger.prepare(root.selectedFingerprint, composer.text)) composer.clear()
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true; Layout.preferredHeight: planRow.implicitHeight + 16; color: "#fff4ce"; radius: 5
                                visible: currentWallet && currentWallet.messenger.preparedTransactionCount > 0
                                RowLayout { id: planRow; anchors.fill: parent; anchors.margins: 8
                                    Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#5f4b00"; text: currentWallet ? qsTr("Encrypted message ready for %1: %2 transaction(s), total fee %3 atomic QWC").arg(root.selectedContact ? root.selectedContact.label : "").arg(currentWallet.messenger.preparedTransactionCount).arg(currentWallet.messenger.preparedFee) : "" }
                                    Button { text: qsTr("Send encrypted message"); onClicked: currentWallet.messenger.commitPrepared() }
                                    Button { text: qsTr("Cancel and delete draft"); onClicked: currentWallet.messenger.cancelPrepared() }
                                }
                            }
                            Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#667781"; text: currentWallet ? currentWallet.messenger.status : "" }
                        }
                    }
                }
            }
        }
    }
}
