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
            { "label": "Alice", "fingerprint": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "confirmed": true },
            { "label": "Bob", "fingerprint": "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", "confirmed": false }
        ]
        property var messages: [
            { "id": "1", "contact": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "label": "Alice", "text": "outgoing", "direction": "out", "status": "sent", "timestamp": "2026-09-19T12:30:00Z" },
            { "id": "2", "contact": "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", "label": "Bob", "text": "incoming", "direction": "in", "status": "confirmed", "timestamp": "2026-09-19T12:31:00Z" }
        ]
        property int preparedTransactionCount: 0
        property int preparedFee: 0
        property string preparedContactFingerprint: ""
        property string status: ""
        property bool canCancelPrepared: true
        function importInvitation(label, invitation) { return true }
        function confirmContact(fingerprint, confirmation) { return fingerprint === confirmation }
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
            page.showContactManagement = false
            wait(0)
        }

        function test_conversation_is_filtered_by_selected_contact() {
            compare(page.filteredMessages.length, 0)
            page.selectedFingerprint = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            compare(page.selectedContact.label, "Alice")
            compare(page.filteredMessages.length, 1)
            compare(page.filteredMessages[0].text, "outgoing")
            page.selectedFingerprint = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
            compare(page.selectedContact.label, "Bob")
            compare(page.filteredMessages.length, 0)
        }

        function test_status_and_timestamp_labels_are_human_readable() {
            compare(page.statusLabel("broadcast"), "Sent")
            compare(page.statusLabel("prepared"), "Ready to send")
            verify(page.timestampLabel("2026-09-19T12:30:00Z").length > 0)
            compare(page.timestampLabel(""), "")
        }

        function test_composer_has_usable_height() {
            page.selectedFingerprint = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            wait(0)
            var composer = findChild(page, "messengerComposer")
            verify(composer !== null)
            verify(composer.height >= 90)
            compare(composer.enabled, true)
        }

        function test_unverified_contact_cannot_compose() {
            page.selectedFingerprint = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
            wait(0)
            var composer = findChild(page, "messengerComposer")
            var prepareButton = findChild(page, "messengerPrepareButton")
            verify(composer !== null)
            verify(prepareButton !== null)
            compare(composer.enabled, false)
            compare(prepareButton.enabled, false)
            compare(page.filteredMessages.length, 0)

            page.showContactManagement = true
            wait(0)
            verify(findChild(page, "messengerFingerprintConfirmation") !== null)
            verify(findChild(page, "messengerConfirmFingerprintButton") !== null)
        }

        function test_utf8_length_is_byte_accurate_and_fail_closed() {
            compare(page.utf8Length("QMS1"), 4)
            compare(page.utf8Length("😀"), 4)
            verify(page.utf8Length("\uD800") > 4096)
        }

        function test_chat_is_primary_and_contact_management_is_separate() {
            var contacts = findChild(page, "messengerContacts")
            var manageButton = findChild(page, "messengerContactManagementButton")
            verify(contacts !== null)
            verify(manageButton !== null)
            compare(page.showContactManagement, false)
            compare(contacts.visible, true)

            manageButton.clicked()
            compare(page.showContactManagement, true)
            compare(contacts.visible, false)
        }
    }
}
