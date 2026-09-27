// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.9
import QtTest 1.2
import "../../components" as MoneroComponents

Item {
    width: 800
    height: 600

    MoneroComponents.RemoteNodeEdit {
        id: editor
    }

    TestCase {
        name: "RemoteNodeEdit"
        when: windowShown

        function init() {
            editor.daemonAddrText = ""
            editor.daemonPortText = ""
        }

        function test_bare_onion_host_gets_explicit_http_scheme() {
            editor.daemonAddrText = "6xasbqd2xkjpgz744bl5jhmlo5qpj7oclibl567jd2xahi2xcxo7z2ad.onion"
            editor.daemonPortText = "8197"
            compare(editor.getAddress(),
                    "http://6xasbqd2xkjpgz744bl5jhmlo5qpj7oclibl567jd2xahi2xcxo7z2ad.onion:8197")
        }

        function test_explicit_onion_scheme_is_preserved() {
            editor.daemonAddrText = "https://6xasbqd2xkjpgz744bl5jhmlo5qpj7oclibl567jd2xahi2xcxo7z2ad.onion"
            editor.daemonPortText = "8197"
            compare(editor.getAddress(),
                    "https://6xasbqd2xkjpgz744bl5jhmlo5qpj7oclibl567jd2xahi2xcxo7z2ad.onion:8197")
        }

        function test_legacy_clearnet_host_is_unchanged() {
            editor.daemonAddrText = "seed.example.org"
            editor.daemonPortText = "8198"
            compare(editor.getAddress(), "seed.example.org:8198")
        }
    }
}
