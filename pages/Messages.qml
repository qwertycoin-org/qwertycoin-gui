import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

Item {
    id: root
    property int contentHeight: page.implicitHeight + 80
    property string selectedFingerprint: ""
    ColumnLayout {
        id: page; width: parent.width; spacing: 12
        Label { text: qsTr("Messages"); font.pixelSize: 28; font.bold: true }
        Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: qsTr("Experimental encrypted QMS1 messenger. No forward secrecy or post-quantum protection. Invitations contain a confidential discovery secret.") }
        GroupBox { title: qsTr("My personal invitation"); Layout.fillWidth: true
            ColumnLayout { anchors.fill: parent
                TextField { Layout.fillWidth: true; readOnly: true; text: currentWallet ? currentWallet.messenger.ownFingerprint : ""; selectByMouse: true }
                TextArea { Layout.fillWidth: true; readOnly: true; wrapMode: TextEdit.WrapAnywhere; text: currentWallet ? currentWallet.messenger.ownInvitation : ""; selectByMouse: true }
            }
        }
        GroupBox { title: qsTr("Import contact invitation"); Layout.fillWidth: true
            ColumnLayout { anchors.fill: parent
                TextField { id: contactLabel; Layout.fillWidth: true; placeholderText: qsTr("Contact name") }
                TextArea { id: contactInvitation; Layout.fillWidth: true; placeholderText: qsTr("Invitation hex"); wrapMode: TextEdit.WrapAnywhere }
                Button { text: qsTr("Import and confirm fingerprint"); onClicked: if (currentWallet.messenger.importInvitation(contactLabel.text, contactInvitation.text)) { contactInvitation.clear(); contactLabel.clear() } }
            }
        }
        RowLayout { Layout.fillWidth: true
            ListView { id: contacts; Layout.preferredWidth: 220; Layout.preferredHeight: 300; model: currentWallet ? currentWallet.messenger.contacts : []
                delegate: Button { width: contacts.width; text: modelData.label + "\n" + modelData.fingerprint.substring(0, 16) + "…"; onClicked: root.selectedFingerprint = modelData.fingerprint }
            }
            ListView { Layout.fillWidth: true; Layout.preferredHeight: 300; model: currentWallet ? currentWallet.messenger.messages : []
                delegate: Rectangle { width: parent.width; height: messageText.implicitHeight + 28; color: modelData.direction === "out" ? "#274257" : "#31513a"; radius: 5
                    Text { id: messageText; anchors.fill: parent; anchors.margins: 10; color: "white"; wrapMode: Text.WordWrap; text: modelData.label + ": " + modelData.text + "\n[" + modelData.status + "]" }
                }
            }
        }
        TextArea { id: composer; Layout.fillWidth: true; placeholderText: qsTr("Message (maximum 4,096 UTF-8 bytes)"); wrapMode: TextEdit.Wrap }
        Label { text: qsTr("UTF-8 bytes: %1 / 4096").arg(unescape(encodeURIComponent(composer.text)).length); color: unescape(encodeURIComponent(composer.text)).length > 4096 ? "red" : "white" }
        RowLayout {
            Button { text: qsTr("Prepare"); enabled: root.selectedFingerprint !== "" && unescape(encodeURIComponent(composer.text)).length <= 4096; onClicked: currentWallet.messenger.prepare(root.selectedFingerprint, composer.text) }
            Label { text: currentWallet ? qsTr("%1 transactions · fee %2 atomic QWC").arg(currentWallet.messenger.preparedTransactionCount).arg(currentWallet.messenger.preparedFee) : "" }
            Button { text: qsTr("Send prepared"); enabled: currentWallet && currentWallet.messenger.preparedTransactionCount > 0; onClicked: currentWallet.messenger.commitPrepared() }
            Button { text: qsTr("Cancel"); onClicked: currentWallet.messenger.cancelPrepared() }
        }
        Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; text: currentWallet ? currentWallet.messenger.status : "" }
    }
}
