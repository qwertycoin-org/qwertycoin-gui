// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.9
import QtTest 1.2
import "../../pages"

Item {
    id: appWindow
    width: 1180
    height: 900

    QtObject {
        id: messengerMock
        property string ownFingerprint: "self-fingerprint"
        property string ownInvitation: "long-personal-invitation"
        property var contacts: [
            { "label": "Alice", "fingerprint": "alice-fingerprint" },
            { "label": "Bob", "fingerprint": "bob-fingerprint" }
        ]
        property var messages: [
            { "id": "1", "contact": "alice-fingerprint", "label": "Alice", "text": "outgoing", "direction": "out", "status": "sent", "timestamp": "2026-09-19T12:30:00Z" },
            { "id": "2", "contact": "bob-fingerprint", "label": "Bob", "text": "incoming", "direction": "in", "status": "confirmed", "timestamp": "2026-09-19T12:31:00Z" }
        ]
        property int preparedTransactionCount: 0
        property int preparedFee: 0
        property string preparedContactFingerprint: ""
        property string status: ""
        function importInvitation(label, invitation) { return true }
        function renameContact(fingerprint, label) { return true }
        function removeContact(fingerprint) { return true }
        function prepare(fingerprint, text) { return true }
        function commitPrepared() { return true }
        function cancelPrepared() {}
    }
    QtObject { id: walletMock; property var messenger: messengerMock }
    property var currentWallet: walletMock

    Messages { id: page; anchors.fill: parent }

    TestCase {
        name: "MessagesPage"
        when: windowShown

        function init() {
            page.selectedFingerprint = ""
            wait(0)
        }

        function test_conversation_is_filtered_by_selected_contact() {
            compare(page.filteredMessages.length, 0)
            page.selectedFingerprint = "alice-fingerprint"
            compare(page.selectedContact.label, "Alice")
            compare(page.filteredMessages.length, 1)
            compare(page.filteredMessages[0].text, "outgoing")
            page.selectedFingerprint = "bob-fingerprint"
            compare(page.selectedContact.label, "Bob")
            compare(page.filteredMessages.length, 1)
            compare(page.filteredMessages[0].text, "incoming")
        }

        function test_status_and_timestamp_labels_are_human_readable() {
            compare(page.statusLabel("broadcast"), "Sent")
            compare(page.statusLabel("prepared"), "Ready to send")
            verify(page.timestampLabel("2026-09-19T12:30:00Z").length > 0)
            compare(page.timestampLabel(""), "")
        }

        function test_composer_has_usable_height() {
            page.selectedFingerprint = "alice-fingerprint"
            wait(0)
            var composer = findChild(page, "messengerComposer")
            verify(composer !== null)
            verify(composer.height >= 90)
        }
    }
}
