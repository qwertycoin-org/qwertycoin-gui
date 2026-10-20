// Copyright (c) 2026 The Qwertycoin Project
// Distributed under the conditions in the project LICENSE file.

import QtQuick 2.9
import QtTest 1.2

// Read the generated resource used by the packaged settings UI and updater.
import "qrc:/version.js" as Version

TestCase {
    name: "VersionIdentity"

    function test_gui_and_linked_core_product_versions_agree() {
        compare(Version.GUI_VERSION_NUMBER, embeddedCoreProductVersion)
        verify(embeddedCoreVersion.indexOf(embeddedCoreProductVersion + "-") === 0)
        verify(Version.GUI_VERSION.indexOf(Version.GUI_VERSION_NUMBER + "-") === 0)
    }

    function test_legacy_compatibility_version_keeps_four_components() {
        compare(embeddedCoreCompatibleVersion, embeddedCoreProductVersion + ".0")
        verify(/^\d+\.\d+\.\d+\.0$/.test(embeddedCoreCompatibleVersion))
        verify(legacyCoreVersion.indexOf(embeddedCoreCompatibleVersion + "-") === 0)
    }
}
