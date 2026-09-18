// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

#include <QtTest>

#include "qt/UpdateMetadata.h"

namespace
{
    constexpr const char hashA[] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    constexpr const char hashB[] = "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789";
}

class UpdateMetadataTests : public QObject
{
    Q_OBJECT

private slots:
    void selectsHighestNewerVersion()
    {
        const auto result = UpdateMetadata::evaluate({
            std::string("qwertycoin-gui:linux-x64:2.0.2:") + hashA,
            std::string("qwertycoin-gui:linux-x64:2.1.0:") + hashB,
            std::string("qwertycoin-gui:win-x64:9.0.0:") + hashA,
            std::string("qwertycoin:linux-x64:9.0.0:") + hashA}, "linux-x64", "2.0.1");
        QVERIFY(result.available);
        QCOMPARE(QString::fromStdString(result.version), QStringLiteral("2.1.0"));
        QCOMPARE(QString::fromStdString(result.hash), QString::fromLatin1(hashB));
    }

    void ignoresPlaceholderAndCurrentVersion()
    {
        QVERIFY(!UpdateMetadata::evaluate({"qwc:update-metadata-not-yet-published"}, "linux-x64", "2.0.1").available);
        QVERIFY(!UpdateMetadata::evaluate({std::string("qwertycoin-gui:linux-x64:2.0.1:") + hashA},
            "linux-x64", "2.0.1").available);
    }

    void rejectsConflictsAndMalformedMatchingRecords()
    {
        QVERIFY(!UpdateMetadata::evaluate({
            std::string("qwertycoin-gui:linux-x64:2.0.2:") + hashA,
            std::string("qwertycoin-gui:linux-x64:2.0.2:") + hashB}, "linux-x64", "2.0.1").available);
        QVERIFY(!UpdateMetadata::evaluate({"qwertycoin-gui:linux-x64:2.0.2"}, "linux-x64", "2.0.1").available);
        QVERIFY(!UpdateMetadata::evaluate({std::string("qwertycoin-gui:linux-x64:2.0.2.0:") + hashA},
            "linux-x64", "2.0.1").available);
        QVERIFY(!UpdateMetadata::evaluate({std::string("qwertycoin-gui:linux-x64:02.0.2:") + hashA},
            "linux-x64", "2.0.1").available);
        QVERIFY(!UpdateMetadata::evaluate({std::string("qwertycoin-gui:linux-x64:2.0.2:") +
            "0123456789ABCDEF0123456789abcdef0123456789abcdef0123456789abcdef"},
            "linux-x64", "2.0.1").available);
        QVERIFY(!UpdateMetadata::evaluate({
            std::string("qwertycoin-gui:linux-x64:2.0.2:") + hashA,
            "qwertycoin-gui:linux-x64:2.0.3:not-a-sha256"},
            "linux-x64", "2.0.1").available);
    }

    void mapsOnlySupportedAssets()
    {
        QCOMPARE(QString::fromStdString(UpdateMetadata::downloadUrl("linux-x64", "2.0.2")),
            QStringLiteral("https://github.com/qwertycoin-org/qwertycoin-gui/releases/download/v2.0.2/qwertycoin-gui-v2.0.2-linux-x86_64.tar.gz"));
        QCOMPARE(QString::fromStdString(UpdateMetadata::downloadUrl("mac-armv8", "2.0.2")),
            QStringLiteral("https://github.com/qwertycoin-org/qwertycoin-gui/releases/download/v2.0.2/qwertycoin-gui-v2.0.2-macos-arm64.dmg"));
        QCOMPARE(QString::fromStdString(UpdateMetadata::downloadUrl("install-win-x64", "2.0.2")),
            QStringLiteral("https://github.com/qwertycoin-org/qwertycoin-gui/releases/download/v2.0.2/qwertycoin-gui-v2.0.2-windows-x86_64-setup.exe"));
        QCOMPARE(QString::fromStdString(UpdateMetadata::downloadUrl("win-x64", "2.0.2")),
            QStringLiteral("https://github.com/qwertycoin-org/qwertycoin-gui/releases/download/v2.0.2/qwertycoin-gui-v2.0.2-windows-x86_64.zip"));
        QVERIFY(UpdateMetadata::downloadUrl("mac-x64", "2.0.2").empty());
        QVERIFY(UpdateMetadata::downloadUrl("source", "2.0.2").empty());
        QVERIFY(UpdateMetadata::downloadUrl("linux-x64", "../../evil").empty());
    }
};

QTEST_GUILESS_MAIN(UpdateMetadataTests)
#include "tst_UpdateMetadata.moc"
