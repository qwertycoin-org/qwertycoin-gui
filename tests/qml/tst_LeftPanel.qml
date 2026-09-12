// Copyright (c) 2026 The Qwertycoin Project
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the conditions in the project
// LICENSE file are met.

import QtQuick 2.9
import QtTest 1.2

import moneroComponents.NetworkType 1.0

import "../.." as AppComponents

Item {
    id: appWindow
    width: 300
    height: 800

    property bool viewOnly: false
    property bool isMac: false
    property bool disconnected: true
    property bool ctrlPressed: false
    property bool themeTransition: false
    property int walletMode: 2

    function fiatApiCurrencySymbol() { return "$" }
    function showPageRequest(page) {}
    function showStatusMessage(message, seconds) {}

    QtObject {
        id: persistentSettings
        property bool customDecorations: true
        property int nettype: NetworkType.MAINNET
        property bool fiatPriceEnabled: false
        property bool fiatPriceToggle: false
    }

    AppComponents.LeftPanel {
        id: panel
        anchors.fill: parent
        currentAccountIndex: 0
        currentAccountLabel: "Primary account"
        balanceString: "0.00000000"
        balanceUnlockedString: "0.00000000"
    }

    TestCase {
        name: "LeftPanel"
        when: windowShown

        function test_wallet_summary_places_currency_after_amount() {
            var whole = findChild(panel, "walletSummaryAmountWhole")
            var fraction = findChild(panel, "walletSummaryAmountFraction")
            var currency = findChild(panel, "walletSummaryCurrency")

            verify(whole !== null)
            verify(fraction !== null)
            verify(currency !== null)
            compare(whole.text, "0.")
            compare(fraction.text, "00000000")
            compare(currency.text, "QWC")
            verify(currency.x >= fraction.x + fraction.width)
        }
    }
}
