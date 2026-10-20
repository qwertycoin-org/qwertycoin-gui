// Copyright (c) 2026 The Qwertycoin Project
// Distributed under the conditions in the project LICENSE file.

import QtQuick 2.9
import QtTest 1.2

TestCase {
    name: "NativeMessenger"

    function test_passwordless_wallet_fails_closed() {
        verify(nativeMessengerTests.passwordGate())
    }

    function test_identity_and_verified_contacts_survive_encrypted_cache_reopen() {
        verify(nativeMessengerTests.encryptedStateReopens())
    }

    function test_close_wallet_disposes_prepared_plan_before_core_wallet() {
        verify(nativeMessengerTests.closeDisposesPreparedPlan())
    }
}
