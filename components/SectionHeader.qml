// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.9
import QtQuick.Layouts 1.1

import "." as MoneroComponents

ColumnLayout {
    id: header
    property alias title: titleLabel.text
    property alias description: descriptionLabel.text
    spacing: MoneroComponents.Style.spaceXs

    MoneroComponents.TextPlain {
        id: titleLabel
        Layout.fillWidth: true
        color: MoneroComponents.Style.defaultFontColor
        font.family: MoneroComponents.Style.fontDisplay.name
        font.pixelSize: 28
        font.bold: true
        wrapMode: Text.Wrap
    }

    MoneroComponents.TextPlain {
        id: descriptionLabel
        Layout.fillWidth: true
        visible: text.length > 0
        color: MoneroComponents.Style.textSecondaryColor
        font.family: MoneroComponents.Style.fontRegular.name
        font.pixelSize: 14
        lineHeight: 1.25
        wrapMode: Text.Wrap
    }
}
