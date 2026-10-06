// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.9
import QtTest 1.2
import "../../js/QmsTransactionHistory.js" as QmsTransactionHistory

Item {
    TestCase {
        name: "QmsTransactionHistory"

        readonly property string carrierA: "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        readonly property string carrierB: "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
        readonly property string paymentId: "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"

        function transaction(hash, amount, confirmations) {
            return {
                "hash": hash,
                "amount": amount,
                "displayAmount": amount + " QWC",
                "fee": "0.00010000",
                "isPending": false,
                "isFailed": false,
                "confirmations": confirmations,
                "blockheight": 100,
                "timestamp": 100,
                "dateHuman": "now",
                "dateTime": "2026-10-06 19:00:00",
                "address": "QWC-address",
                "addressBookName": "",
                "tx_note": ""
            };
        }

        function transactionWithFee(hash, amount, confirmations, fee) {
            var result = transaction(hash, amount, confirmations);
            result.fee = fee;
            return result;
        }

        function messageGroup(ids) {
            return {
                "messageId": "message-1",
                "transactionIds": ids,
                "transactionCount": ids.length,
                "fee": "0.00020000",
                "status": "sent"
            };
        }

        function test_exact_hashes_are_grouped_without_carrier_amount() {
            var grouped = QmsTransactionHistory.groupTransactions([
                transaction(carrierA.toUpperCase(), 0.00000001, 12),
                transaction(carrierB, 0.00000001, 9),
                transaction(paymentId, 4.5, 20)
            ], [messageGroup([carrierA, carrierB])]);

            compare(grouped.length, 2);
            compare(grouped[0].isMessenger, true);
            compare(grouped[0].amount, 0);
            compare(grouped[0].displayAmount, "");
            compare(grouped[0].fee, "0.00020000");
            compare(grouped[0].messengerTransactionCount, 2);
            compare(grouped[0].messengerMatchedTransactionCount, 2);
            compare(grouped[0].confirmations, 9);
            compare(grouped[0].messengerTransactionIdsText, carrierA + "\n" + carrierB);
            compare(grouped[0].fee, "0.00020000");
            compare(grouped[1].isMessenger, false);
            compare(grouped[1].amount, 4.5);
        }

        function test_partial_history_uses_only_matched_carriers_and_fees() {
            var grouped = QmsTransactionHistory.groupTransactions([
                transactionWithFee(carrierA, 0.00000001, 12, "0.00030001")
            ], [{
                "messageId": "message-1",
                "transactionIds": [carrierA, carrierB],
                "transactionCount": 2,
                "fee": "99.00000000",
                "status": "broadcast outcome unknown"
            }]);

            compare(grouped.length, 1);
            compare(grouped[0].isMessenger, true);
            compare(grouped[0].messengerTransactionCount, 1);
            compare(grouped[0].messengerMatchedTransactionCount, 1);
            compare(grouped[0].messengerTransactionIdsText, carrierA);
            compare(grouped[0].fee, "0.00030001");
        }

        function test_one_atomic_unit_is_never_classified_by_amount() {
            var grouped = QmsTransactionHistory.groupTransactions([
                transaction(paymentId, 0.00000001, 20)
            ], [messageGroup([carrierA])]);

            compare(grouped.length, 1);
            compare(grouped[0].isMessenger, false);
            compare(grouped[0].amount, 0.00000001);
        }

        function test_ambiguous_hash_claims_fail_open_as_payments() {
            var grouped = QmsTransactionHistory.groupTransactions([
                transaction(carrierA, 0.00000001, 20)
            ], [
                messageGroup([carrierA]),
                { "messageId": "message-2", "transactionIds": [carrierA], "fee": "0.1", "status": "sent" }
            ]);

            compare(grouped.length, 1);
            compare(grouped[0].isMessenger, false);
        }

        function test_malformed_group_claims_fail_open_as_payments() {
            var grouped = QmsTransactionHistory.groupTransactions([
                transaction(carrierA, 0.00000001, 20)
            ], [
                messageGroup([carrierA]),
                { "messageId": "message-2", "transactionIds": [carrierA, carrierA], "fee": "0.1", "status": "sent" }
            ]);

            compare(grouped.length, 1);
            compare(grouped[0].isMessenger, false);
        }

        function test_duplicate_history_rows_fail_open() {
            var grouped = QmsTransactionHistory.groupTransactions([
                transaction(carrierA, 0.00000001, 20),
                transaction(carrierA, 0.00000001, 20)
            ], [messageGroup([carrierA])]);

            compare(grouped.length, 2);
            compare(grouped[0].isMessenger, false);
            compare(grouped[1].isMessenger, false);
        }

        function test_category_filters_preserve_auditable_access() {
            var grouped = QmsTransactionHistory.groupTransactions([
                transaction(carrierA, 0.00000001, 20),
                transaction(paymentId, 2, 20)
            ], [messageGroup([carrierA])]);

            compare(QmsTransactionHistory.filterTransactions(grouped, "all").length, 2);
            compare(QmsTransactionHistory.filterTransactions(grouped, "payments").length, 1);
            compare(QmsTransactionHistory.filterTransactions(grouped, "payments")[0].hash, paymentId);
            compare(QmsTransactionHistory.filterTransactions(grouped, "messenger").length, 1);
            compare(QmsTransactionHistory.filterTransactions(grouped, "messenger")[0].isMessenger, true);
        }
    }
}
