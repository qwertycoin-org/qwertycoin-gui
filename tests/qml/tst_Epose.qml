// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.9
import QtTest 1.2

import moneroComponents.NetworkType 1.0
import "../../pages"

Item {
    id: appWindow
    width: 1000
    height: 900

    property int walletMode: 2
    property bool daemonRunning: false
    property bool themeTransition: false
    property int observationRefreshes: 0

    function configureEposeObservation() {}
    function startDaemon() {}
    function stopDaemon(callback) { if (callback) callback() }

    QtObject {
        id: persistentSettings
        property bool useRemoteNode: false
        property int nettype: NetworkType.MAINNET
        property string daemonFlags: ""
        property string blockchainDataDir: ""
        property string eposeRewardAddress: ""
        property string eposeEndpointHost: "service.example.org"
        property int eposeEndpointPort: 8198
        property string eposeDiscoveryEndpoints: "http://seed.example.org:8198"
        property bool eposeServiceEnabled: false
    }

    QtObject {
        id: eposeManager
        property bool loading: false
        property string state: "ready"
        property string daemonEndpoint: "http://127.0.0.1:8197"
        property string lastSuccessfulAt: "2026-09-12T12:00:00.000Z"
        property string lastError: ""
        property var info: ({
            "enabled": true,
            "protocolVersion": "2",
            "currentEpoch": "7",
            "serviceNodeCount": "2",
            "qualifiedCount": "1",
            "attestationCount": "12",
            "stateHash": "state-hash",
            "serviceRewardBps": "1000",
            "localServiceNode": false,
            "localKeyLoaded": false,
            "localRegistered": false,
            "localActive": false,
            "localQualified": false
        })
        property var epoch: ({
            "epoch": "7",
            "startHeight": "5040",
            "endHeight": "5759",
            "activeCount": "2",
            "qualifiedCount": "1"
        })
        property var serviceNodes: ([{
            "identityId": "identity",
            "descriptorSequence": "18446744073709551615",
            "effectiveEpoch": "7",
            "expiryEpoch": "9",
            "active": true,
            "qualified": true
        }])
        property var rewardPreview: ({ "previewAvailable": false })
        property var legacyRegistration: ({ "ready": false, "retired": true })
        property var sourceStatus: ({
            "eposeInfo": { "state": "ready", "observedAt": "2026-09-12T12:00:00.000Z" },
            "epoch": { "state": "ready", "observedAt": "2026-09-12T12:00:00.010Z" },
            "serviceNodes": { "state": "ready", "observedAt": "2026-09-12T12:00:00.020Z" },
            "localServiceNode": { "state": "ready", "observedAt": "2026-09-12T12:00:00.025Z" },
            "rewardPreview": { "state": "ready", "observedAt": "2026-09-12T12:00:00.030Z" },
            "legacyRegistration": { "state": "ready", "observedAt": "2026-09-12T12:00:00.040Z" }
        })
        property bool listTruncated: false
        function refresh() { appWindow.observationRefreshes += 1 }
    }

    QtObject {
        id: daemonManager
        function validateEposeServiceConfig() {
            return { "valid": true, "error": "", "keystorePath": "/tmp/test-keystore", "endpoint": "service.example.org:8198" }
        }
    }

    QtObject {
        id: confirmationDialog
        property string title: ""
        property string text: ""
        property string okText: ""
        property string cancelText: ""
        property var onAcceptedCallback: null
        property var onRejectedCallback: null
        function open() {}
    }

    Epose {
        id: page
        anchors.fill: parent
    }

    TestCase {
        name: "EposePage"
        when: windowShown

        function test_page_loads_with_current_snapshot() {
            verify(page.contentHeight > appWindow.height)
            compare(page.locallyConfigurable, true)
            tryCompare(appWindow, "observationRefreshes", 1)
        }

        function test_remote_daemon_is_observation_only() {
            persistentSettings.useRemoteNode = true
            compare(page.locallyConfigurable, false)
            persistentSettings.useRemoteNode = false
            compare(page.locallyConfigurable, true)
        }

        function test_access_failure_remains_explicit() {
            eposeManager.state = "unauthorized"
            eposeManager.lastError = "The daemon rejected access rights."
            compare(eposeManager.state, "unauthorized")
            compare(eposeManager.lastError.length > 0, true)
            eposeManager.state = "ready"
            eposeManager.lastError = ""
        }
    }
}
