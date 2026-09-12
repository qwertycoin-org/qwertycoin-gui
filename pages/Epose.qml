// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.9
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.2

import "../components" as MoneroComponents

Item {
    id: root
    property int contentHeight: 0
    property bool contentHeightUpdatePending: false
    property string setupError: ""
    property bool locallyConfigurable: !persistentSettings.useRemoteNode && appWindow.walletMode >= 2

    // TranslationManager exposes a notifier-only empty string so Qt 5 QML
    // bindings are re-evaluated after a runtime language change.
    function localized(text) {
        translationManager.emptyString
        return text
    }

    function value(map, key, fallback) {
        return map && map[key] !== undefined && map[key] !== "" ? map[key] : fallback
    }

    function compact(value) {
        if (!value || value.length <= 22)
            return value || root.localized(qsTr("Unknown"))
        return value.slice(0, 10) + "…" + value.slice(-10)
    }

    function sourceTone(state) {
        if (state === "ready")
            return "success"
        if (state === "unsupported" || state === "unknown")
            return "neutral"
        if (state === "unauthorized" || state === "error")
            return "error"
        return "warning"
    }

    function sourceStateLabel(state) {
        if (state === "ready")
            return root.localized(qsTr("Current"))
        if (state === "unsupported")
            return root.localized(qsTr("Not supported"))
        if (state === "unauthorized")
            return root.localized(qsTr("Not authorized"))
        if (state === "error")
            return root.localized(qsTr("Failed"))
        return root.localized(qsTr("Not queried"))
    }

    function scheduleContentHeightUpdate() {
        if (contentHeightUpdatePending)
            return
        contentHeightUpdatePending = true
        Qt.callLater(function() {
            root.contentHeight = Math.ceil(contentColumn.implicitHeight + MoneroComponents.Style.space2Xl)
            root.contentHeightUpdatePending = false
        })
    }

    function onPageCompleted() {
        scheduleContentHeightUpdate()
        appWindow.configureEposeObservation()
        eposeManager.refresh()
    }

    function onPageClosed() {
        refreshTimer.stop()
    }

    function restartLocalDaemon() {
        const restart = function() { appWindow.startDaemon(persistentSettings.daemonFlags) }
        if (appWindow.daemonRunning)
            appWindow.stopDaemon(restart, true)
        else
            restart()
    }

    function validateAndConfirm() {
        const result = daemonManager.validateEposeServiceConfig(
            persistentSettings.daemonFlags,
            persistentSettings.nettype,
            persistentSettings.blockchainDataDir,
            rewardAddress.text,
            endpointHost.text,
            parseInt(endpointPort.text),
            discoveryEndpoints.text)
        setupError = result.error || ""
        if (!result.valid)
            return

        confirmationDialog.title = root.localized(qsTr("Enable local EPoSe service"))
        confirmationDialog.text = root.localized(qsTr("The local daemon will restart as a continuously available EPoSe service. It will create or load a separate, chain-bound operator/service keystore at:\n\n%1\n\nThe public restricted-RPC probe endpoint will be %2. No wallet private key is used or stored."))
            .arg(result.keystorePath).arg(result.endpoint)
        confirmationDialog.okText = root.localized(qsTr("Enable and restart"))
        confirmationDialog.cancelText = root.localized(qsTr("Cancel"))
        confirmationDialog.onAcceptedCallback = function() {
            persistentSettings.eposeRewardAddress = rewardAddress.text.trim()
            persistentSettings.eposeEndpointHost = endpointHost.text.trim()
            persistentSettings.eposeEndpointPort = parseInt(endpointPort.text)
            persistentSettings.eposeDiscoveryEndpoints = discoveryEndpoints.text.trim()
            persistentSettings.eposeServiceEnabled = true
            appWindow.configureEposeObservation()
            restartLocalDaemon()
        }
        confirmationDialog.onRejectedCallback = null
        confirmationDialog.open()
    }

    function confirmDisable() {
        confirmationDialog.title = root.localized(qsTr("Stop local EPoSe service"))
        confirmationDialog.text = root.localized(qsTr("The local daemon will restart without the EPoSe producer. The chain-bound operator/service keystore is preserved for a later restart."))
        confirmationDialog.okText = root.localized(qsTr("Stop service"))
        confirmationDialog.cancelText = root.localized(qsTr("Cancel"))
        confirmationDialog.onAcceptedCallback = function() {
            persistentSettings.eposeServiceEnabled = false
            appWindow.configureEposeObservation()
            restartLocalDaemon()
        }
        confirmationDialog.onRejectedCallback = null
        confirmationDialog.open()
    }

    Timer {
        id: refreshTimer
        interval: 30000
        repeat: true
        running: root.visible
        triggeredOnStart: true
        onTriggered: eposeManager.refresh()
    }

    ColumnLayout {
        id: contentColumn
        width: root.width
        spacing: MoneroComponents.Style.spaceLg
        onImplicitHeightChanged: root.scheduleContentHeightUpdate()

        MoneroComponents.SectionHeader {
            Layout.fillWidth: true
            Layout.topMargin: MoneroComponents.Style.spaceMd
            title: root.localized(qsTr("EPoSe"))
            description: root.localized(qsTr("Observe service consensus and configure the supported local producer. RandomX remains responsible for block production and chain selection."))
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: MoneroComponents.Style.spaceMd

            MoneroComponents.StatusChip {
                text: eposeManager.loading ? root.localized(qsTr("Refreshing"))
                      : (eposeManager.state === "ready" ? root.localized(qsTr("Current"))
                         : (eposeManager.state === "partial" ? root.localized(qsTr("Partial data"))
                            : (eposeManager.state === "unsupported" ? root.localized(qsTr("Not supported"))
                               : (eposeManager.state === "unauthorized" ? root.localized(qsTr("Not authorized"))
                                  : (eposeManager.state === "error" ? root.localized(qsTr("Unavailable")) : root.localized(qsTr("Unknown")))))))
                tone: eposeManager.state === "ready" ? "success"
                      : (eposeManager.state === "partial" ? "warning"
                         : ((eposeManager.state === "error" || eposeManager.state === "unauthorized") ? "error" : "neutral"))
            }
            MoneroComponents.TextPlain {
                Layout.fillWidth: true
                text: persistentSettings.useRemoteNode
                      ? root.localized(qsTr("Observed through remote daemon: %1")).arg(eposeManager.daemonEndpoint)
                      : root.localized(qsTr("Observed through the local daemon"))
                color: MoneroComponents.Style.textSecondaryColor
                font.family: MoneroComponents.Style.fontRegular.name
                font.pixelSize: 13
                elide: Text.ElideMiddle
            }
            MoneroComponents.StandardButton {
                small: true
                primary: false
                text: root.localized(qsTr("Refresh"))
                enabled: !eposeManager.loading
                onClicked: eposeManager.refresh()
            }
        }

        MoneroComponents.WarningBox {
            Layout.fillWidth: true
            visible: eposeManager.lastError !== ""
            text: eposeManager.lastError
        }

        GridLayout {
            Layout.fillWidth: true
            columns: root.width >= 860 ? 2 : 1
            columnSpacing: MoneroComponents.Style.spaceLg
            rowSpacing: MoneroComponents.Style.spaceLg

            MoneroComponents.BrandCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                ColumnLayout {
                    anchors.fill: parent
                    spacing: MoneroComponents.Style.spaceMd
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("Current epoch")); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontDisplay.name; font.pixelSize: 20; font.bold: true }
                    MoneroComponents.TextPlain { text: root.value(eposeManager.epoch, "epoch", root.value(eposeManager.info, "currentEpoch", "—")); color: MoneroComponents.Style.accentGold; font.family: MoneroComponents.Style.fontMonoRegular.name; font.pixelSize: 32; font.bold: true }
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("Blocks %1–%2")).arg(root.value(eposeManager.epoch, "startHeight", "—")).arg(root.value(eposeManager.epoch, "endHeight", "—")); color: MoneroComponents.Style.textSecondaryColor; font.family: MoneroComponents.Style.fontMonoRegular.name; font.pixelSize: 14 }
                    RowLayout {
                        spacing: MoneroComponents.Style.spaceSm
                        MoneroComponents.StatusChip { text: root.localized(qsTr("%1 active")).arg(root.value(eposeManager.epoch, "activeCount", "—")); tone: "info" }
                        MoneroComponents.StatusChip { text: root.localized(qsTr("%1 qualified")).arg(root.value(eposeManager.epoch, "qualifiedCount", "—")); tone: "success" }
                    }
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("State %1")).arg(root.compact(root.value(eposeManager.info, "stateHash", ""))); color: MoneroComponents.Style.textSecondaryColor; font.family: MoneroComponents.Style.fontMonoRegular.name; font.pixelSize: 13; elide: Text.ElideMiddle }
                    Item { Layout.fillHeight: true }
                }
            }

            MoneroComponents.BrandCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                ColumnLayout {
                    anchors.fill: parent
                    spacing: MoneroComponents.Style.spaceMd
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("Network view")); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontDisplay.name; font.pixelSize: 20; font.bold: true }
                    MoneroComponents.StatusChip { text: root.value(eposeManager.info, "enabled", false) ? root.localized(qsTr("EPoSe enabled")) : root.localized(qsTr("EPoSe not active")); tone: root.value(eposeManager.info, "enabled", false) ? "success" : "neutral" }
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("Protocol %1 · Reward %2 bps")).arg(root.value(eposeManager.info, "protocolVersion", "—")).arg(root.value(eposeManager.info, "serviceRewardBps", "—")); color: MoneroComponents.Style.textSecondaryColor; font.family: MoneroComponents.Style.fontMonoRegular.name; font.pixelSize: 14 }
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("%1 service nodes · %2 attestations")).arg(root.value(eposeManager.info, "serviceNodeCount", "—")).arg(root.value(eposeManager.info, "attestationCount", "—")); color: MoneroComponents.Style.defaultFontColor; font.pixelSize: 15 }
                    MoneroComponents.WarningBox { Layout.fillWidth: true; text: root.localized(qsTr("Reward preview is not available in this Core version. This does not mean EPoSe or actual service rewards are disabled.")); visible: eposeManager.rewardPreview.previewAvailable === false }
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("Sources are queried separately; values are not presented as one atomic block snapshot.")); color: MoneroComponents.Style.textSecondaryColor; font.pixelSize: 13; wrapMode: Text.Wrap }
                    Item { Layout.fillHeight: true }
                }
            }
        }

        MoneroComponents.BrandCard {
            Layout.fillWidth: true
            Layout.preferredHeight: sourceList.implicitHeight + MoneroComponents.Style.space2Xl
            ColumnLayout {
                anchors.fill: parent
                spacing: MoneroComponents.Style.spaceMd
                MoneroComponents.TextPlain {
                    Layout.fillWidth: true
                    text: root.localized(qsTr("Observation sources"))
                    color: MoneroComponents.Style.defaultFontColor
                    font.family: MoneroComponents.Style.fontDisplay.name
                    font.pixelSize: 20
                    font.bold: true
                }
                MoneroComponents.TextPlain {
                    Layout.fillWidth: true
                    text: root.localized(qsTr("Each RPC source has its own observation time. A current row does not make the other rows part of the same block snapshot."))
                    color: MoneroComponents.Style.textSecondaryColor
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                }
                ColumnLayout {
                    id: sourceList
                    Layout.fillWidth: true
                    spacing: MoneroComponents.Style.spaceSm
                    Repeater {
                        model: [
                            { "key": "eposeInfo", "label": root.localized(qsTr("Capabilities and status")) },
                            { "key": "epoch", "label": root.localized(qsTr("Current epoch")) },
                            { "key": "serviceNodes", "label": root.localized(qsTr("Service-node list")) },
                            { "key": "localServiceNode", "label": root.localized(qsTr("Connected daemon service entry")) },
                            { "key": "rewardPreview", "label": root.localized(qsTr("Reward preview")) },
                            { "key": "legacyRegistration", "label": root.localized(qsTr("Legacy registration path")) }
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: MoneroComponents.Style.spaceMd
                            property var statusValue: root.value(eposeManager.sourceStatus, modelData.key, {})
                            property string sourceState: root.value(statusValue, "state", "unknown")
                            MoneroComponents.TextPlain {
                                Layout.preferredWidth: 210
                                text: modelData.label
                                color: MoneroComponents.Style.defaultFontColor
                                font.pixelSize: 13
                            }
                            MoneroComponents.StatusChip {
                                text: root.sourceStateLabel(sourceState)
                                tone: root.sourceTone(sourceState)
                            }
                            MoneroComponents.TextPlain {
                                Layout.fillWidth: true
                                text: root.value(statusValue, "observedAt", root.localized(qsTr("Never")))
                                color: MoneroComponents.Style.textSecondaryColor
                                font.family: MoneroComponents.Style.fontMonoRegular.name
                                font.pixelSize: 12
                                horizontalAlignment: Text.AlignRight
                                elide: Text.ElideLeft
                            }
                        }
                    }
                }
            }
        }

        MoneroComponents.BrandCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 320
            ColumnLayout {
                anchors.fill: parent
                spacing: MoneroComponents.Style.spaceMd
                RowLayout {
                    Layout.fillWidth: true
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("Connected daemon service")); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontDisplay.name; font.pixelSize: 20; font.bold: true }
                    MoneroComponents.StatusChip { text: root.value(eposeManager.info, "localServiceNode", false) ? root.localized(qsTr("Producer reported")) : root.localized(qsTr("Observer only")); tone: root.value(eposeManager.info, "localServiceNode", false) ? "info" : "neutral" }
                }
                MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("This reports what the connected daemon claims. A matching reward address alone does not prove operator control.")); color: MoneroComponents.Style.textSecondaryColor; font.pixelSize: 13; wrapMode: Text.Wrap }
                RowLayout {
                    spacing: MoneroComponents.Style.spaceSm
                    MoneroComponents.StatusChip { text: root.value(eposeManager.info, "localKeyLoaded", false) ? root.localized(qsTr("Key loaded")) : root.localized(qsTr("Key unknown")); tone: root.value(eposeManager.info, "localKeyLoaded", false) ? "success" : "neutral" }
                    MoneroComponents.StatusChip { text: root.localized(qsTr("Registered")); tone: root.value(eposeManager.info, "localRegistered", false) ? "success" : (root.value(eposeManager.info, "localKeyLoaded", false) ? "warning" : "neutral") }
                    MoneroComponents.StatusChip { text: root.localized(qsTr("Active")); tone: root.value(eposeManager.info, "localActive", false) ? "success" : (root.value(eposeManager.info, "localRegistered", false) ? "warning" : "neutral") }
                    MoneroComponents.StatusChip { text: root.localized(qsTr("Qualified")); tone: root.value(eposeManager.info, "localQualified", false) ? "success" : (root.value(eposeManager.info, "localActive", false) ? "warning" : "neutral") }
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: MoneroComponents.Style.spaceLg
                    rowSpacing: MoneroComponents.Style.spaceSm
                    MoneroComponents.TextPlain { text: root.localized(qsTr("Stable identity")); color: MoneroComponents.Style.textSecondaryColor; font.pixelSize: 13 }
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.compact(root.value(eposeManager.info, "localIdentityId", "")); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontMonoRegular.name; font.pixelSize: 13; elide: Text.ElideMiddle }
                    MoneroComponents.TextPlain { text: root.localized(qsTr("Current service key")); color: MoneroComponents.Style.textSecondaryColor; font.pixelSize: 13 }
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.compact(root.value(eposeManager.info, "localServicePublicKey", "")); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontMonoRegular.name; font.pixelSize: 13; elide: Text.ElideMiddle }
                    MoneroComponents.TextPlain { text: root.localized(qsTr("Reward address")); color: MoneroComponents.Style.textSecondaryColor; font.pixelSize: 13 }
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.compact(root.value(eposeManager.info, "localRewardAddress", "")); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontMonoRegular.name; font.pixelSize: 13; elide: Text.ElideMiddle }
                    MoneroComponents.TextPlain { text: root.localized(qsTr("Endpoint")); color: MoneroComponents.Style.textSecondaryColor; font.pixelSize: 13 }
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.value(eposeManager.info, "localAdvertisedEndpoint", "—"); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontMonoRegular.name; font.pixelSize: 13; elide: Text.ElideMiddle }
                    MoneroComponents.TextPlain { text: root.localized(qsTr("Expiry epoch")); color: MoneroComponents.Style.textSecondaryColor; font.pixelSize: 13 }
                    MoneroComponents.TextPlain { text: root.value(eposeManager.info, "localExpiryEpoch", "—"); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontMonoRegular.name; font.pixelSize: 13 }
                    MoneroComponents.TextPlain { text: root.localized(qsTr("Descriptor sequence")); color: MoneroComponents.Style.textSecondaryColor; font.pixelSize: 13 }
                    MoneroComponents.TextPlain { text: root.value(eposeManager.info, "localDescriptorSequence", "—"); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontMonoRegular.name; font.pixelSize: 13 }
                }
                Item { Layout.fillHeight: true }
            }
        }

        MoneroComponents.BrandCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 520
            visible: root.locallyConfigurable
            ColumnLayout {
                anchors.fill: parent
                spacing: MoneroComponents.Style.spaceMd
                RowLayout {
                    Layout.fillWidth: true
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("Local EPoSe producer")); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontDisplay.name; font.pixelSize: 20; font.bold: true }
                    MoneroComponents.StatusChip { text: persistentSettings.eposeServiceEnabled ? root.localized(qsTr("Configured")) : root.localized(qsTr("Off")); tone: persistentSettings.eposeServiceEnabled ? "info" : "neutral" }
                }
                MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("Service mode is opt-in. The Core producer handles admission, registration and renewal. No wallet private view or spend key is required.")); color: MoneroComponents.Style.textSecondaryColor; font.pixelSize: 14; wrapMode: Text.Wrap }
                MoneroComponents.LineEdit { id: rewardAddress; Layout.fillWidth: true; labelText: root.localized(qsTr("Primary public QWC reward address")); placeholderText: root.localized(qsTr("QWC primary address")); text: persistentSettings.eposeRewardAddress }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: MoneroComponents.Style.spaceMd
                    MoneroComponents.LineEdit { id: endpointHost; Layout.fillWidth: true; labelText: root.localized(qsTr("Public endpoint host")); placeholderText: root.localized(qsTr("service.example.org")); text: persistentSettings.eposeEndpointHost }
                    MoneroComponents.LineEdit { id: endpointPort; Layout.preferredWidth: 170; labelText: root.localized(qsTr("Restricted RPC port")); placeholderText: "8198"; text: String(persistentSettings.eposeEndpointPort); validator: IntValidator { bottom: 1; top: 65535 } }
                }
                MoneroComponents.LineEdit { id: discoveryEndpoints; Layout.fillWidth: true; labelText: root.localized(qsTr("Discovery endpoints")); placeholderText: root.localized(qsTr("http://seed-01.example.org:8198, http://seed-02.example.org:8198")); text: persistentSettings.eposeDiscoveryEndpoints; tipText: root.localized(qsTr("Comma-, semicolon- or whitespace-separated public HTTP endpoints.")) }
                MoneroComponents.WarningBox { Layout.fillWidth: true; visible: root.setupError !== ""; text: root.setupError }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: MoneroComponents.Style.spaceMd
                    MoneroComponents.StandardButton { text: persistentSettings.eposeServiceEnabled ? root.localized(qsTr("Validate and restart")) : root.localized(qsTr("Enable EPoSe service")); onClicked: root.validateAndConfirm() }
                    MoneroComponents.StandardButton { visible: persistentSettings.eposeServiceEnabled; primary: false; text: root.localized(qsTr("Stop service")); onClicked: root.confirmDisable() }
                    Item { Layout.fillWidth: true }
                }
            }
        }

        MoneroComponents.WarningBox {
            Layout.fillWidth: true
            visible: persistentSettings.useRemoteNode
            text: root.localized(qsTr("Remote daemon access is observation-only. This wallet cannot start, stop or prove control of that daemon's EPoSe service."))
        }

        MoneroComponents.BrandCard {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(130, serviceList.implicitHeight + MoneroComponents.Style.space2Xl)
            ColumnLayout {
                anchors.fill: parent
                spacing: MoneroComponents.Style.spaceMd
                RowLayout {
                    Layout.fillWidth: true
                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.localized(qsTr("Service-node view")); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontDisplay.name; font.pixelSize: 20; font.bold: true }
                    MoneroComponents.StatusChip { visible: eposeManager.listTruncated; text: root.localized(qsTr("Partial list")); tone: "warning" }
                }
                ColumnLayout {
                    id: serviceList
                    Layout.fillWidth: true
                    spacing: MoneroComponents.Style.spaceSm
                    MoneroComponents.TextPlain { visible: eposeManager.serviceNodes.length === 0; Layout.fillWidth: true; text: root.localized(qsTr("No service-node entries were returned by the connected daemon.")); color: MoneroComponents.Style.textSecondaryColor; font.pixelSize: 14 }
                    Repeater {
                        model: eposeManager.serviceNodes
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 58
                            radius: MoneroComponents.Style.radiusSm
                            color: MoneroComponents.Style.raisedColor
                            border.width: 1
                            border.color: MoneroComponents.Style.borderSubtleColor
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: MoneroComponents.Style.spaceMd
                                spacing: MoneroComponents.Style.spaceMd
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    MoneroComponents.TextPlain { Layout.fillWidth: true; text: root.compact(modelData.identityId); color: MoneroComponents.Style.defaultFontColor; font.family: MoneroComponents.Style.fontMonoRegular.name; font.pixelSize: 13; elide: Text.ElideMiddle }
                                    MoneroComponents.TextPlain { text: root.localized(qsTr("Sequence %1 · effective %2 · expires %3")).arg(modelData.descriptorSequence).arg(modelData.effectiveEpoch).arg(modelData.expiryEpoch); color: MoneroComponents.Style.textSecondaryColor; font.pixelSize: 12 }
                                }
                                MoneroComponents.StatusChip { text: modelData.active ? root.localized(qsTr("Active")) : root.localized(qsTr("Pending")); tone: modelData.active ? "info" : "warning" }
                                MoneroComponents.StatusChip { text: modelData.qualified ? root.localized(qsTr("Qualified")) : root.localized(qsTr("Not qualified")); tone: modelData.qualified ? "success" : "neutral" }
                            }
                        }
                    }
                }
            }
        }

        MoneroComponents.TextPlain {
            Layout.fillWidth: true
            text: eposeManager.lastSuccessfulAt === "" ? root.localized(qsTr("No successful EPoSe observation yet.")) : root.localized(qsTr("Last successful observation: %1")).arg(eposeManager.lastSuccessfulAt)
            color: MoneroComponents.Style.textSecondaryColor
            font.pixelSize: 12
            horizontalAlignment: Text.AlignRight
        }
    }
}
