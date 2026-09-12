// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.9

import "." as MoneroComponents

Rectangle {
    id: chip
    property alias text: label.text
    property string tone: "neutral" // neutral, info, success, warning, error
    property color toneColor: {
        if (tone === "success") return MoneroComponents.Style.successColor
        if (tone === "warning") return MoneroComponents.Style.accentGold
        if (tone === "error") return MoneroComponents.Style.errorColor
        if (tone === "info") return MoneroComponents.Style.accentViolet
        return MoneroComponents.Style.textSecondaryColor
    }

    implicitWidth: label.implicitWidth + MoneroComponents.Style.spaceLg
    implicitHeight: 28
    radius: MoneroComponents.Style.radiusSm
    color: "transparent"
    border.width: 1
    border.color: toneColor

    MoneroComponents.TextPlain {
        id: label
        anchors.centerIn: parent
        color: chip.toneColor
        font.family: MoneroComponents.Style.fontMedium.name
        font.pixelSize: 12
        font.bold: true
    }
}
