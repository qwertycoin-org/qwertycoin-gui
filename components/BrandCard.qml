// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.9

import "." as MoneroComponents

Rectangle {
    id: card

    default property alias content: contentItem.data
    property alias contentItem: contentItem
    property int contentPadding: MoneroComponents.Style.spaceLg
    property bool elevated: false

    // Card content usually anchors to this item's padded content area. Using
    // childrenRect in that case creates a parent/child implicit-size loop.
    implicitWidth: contentItem.children.length > 0
                   ? contentItem.children[0].implicitWidth + contentPadding * 2
                   : contentPadding * 2
    implicitHeight: contentItem.children.length > 0
                    ? contentItem.children[0].implicitHeight + contentPadding * 2
                    : contentPadding * 2
    color: MoneroComponents.Style.cardColor
    radius: MoneroComponents.Style.radiusLg
    border.width: MoneroComponents.Style.contourWidth
    border.color: MoneroComponents.Style.borderSubtleColor

    Rectangle {
        visible: card.elevated
        x: 3
        y: 4
        z: -1
        width: card.width
        height: card.height
        radius: card.radius
        color: MoneroComponents.Style.shadowColor
        opacity: 0.28
    }

    Item {
        id: contentItem
        anchors.fill: parent
        anchors.margins: card.contentPadding
    }
}
