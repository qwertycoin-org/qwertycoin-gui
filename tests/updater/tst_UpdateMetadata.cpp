// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

#include <QtTest>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "qt/DownloadRedirectPolicy.h"
#include "qt/UpdateMetadata.h"
#include "qt/network.h"

QString randomUserAgent()
{
    return QStringLiteral("Qwertycoin updater test");
}

namespace
{
    constexpr const char hashA[] = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    constexpr const char hashB[] = "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789";

    struct FakeResponse
    {
        int status;
        std::string location;
        std::string body;
    };

    class FakeHttpClient final : public epee::net_utils::http::abstract_http_client
    {
    public:
        explicit FakeHttpClient(std::vector<FakeResponse> responses)
            : m_responses(std::move(responses))
        {
        }

        void set_server(
            std::string host,
            std::string port,
            boost::optional<epee::net_utils::http::login>,
            epee::net_utils::ssl_options_t) override
        {
            m_hosts.push_back(std::move(host));
            m_ports.push_back(std::move(port));
        }

        void set_auto_connect(bool) override {}
        bool connect(std::chrono::milliseconds) override { return true; }
        bool disconnect() override { return true; }
        bool is_connected(bool *) override { return true; }

        bool invoke(
            const boost::string_ref uri,
            const boost::string_ref,
            const boost::string_ref,
            std::chrono::milliseconds,
            const epee::net_utils::http::http_response_info **responseInfo,
            const epee::net_utils::http::fields_list &) override
        {
            if (m_nextResponse >= m_responses.size())
                return false;

            m_uris.emplace_back(uri.data(), uri.size());
            const FakeResponse &next = m_responses[m_nextResponse++];
            m_response.clear();
            m_response.m_response_code = next.status;
            m_response.m_body = next.body;
            if (!next.location.empty())
                m_response.m_header_info.m_etc_fields.emplace_back("Location", next.location);
            if (responseInfo)
                *responseInfo = &m_response;
            return true;
        }

        bool invoke_get(
            const boost::string_ref uri,
            std::chrono::milliseconds timeout,
            const std::string &body,
            const epee::net_utils::http::http_response_info **responseInfo,
            const epee::net_utils::http::fields_list &fields) override
        {
            return invoke(uri, "GET", body, timeout, responseInfo, fields);
        }

        bool invoke_post(
            const boost::string_ref uri,
            const std::string &body,
            std::chrono::milliseconds timeout,
            const epee::net_utils::http::http_response_info **responseInfo,
            const epee::net_utils::http::fields_list &fields) override
        {
            return invoke(uri, "POST", body, timeout, responseInfo, fields);
        }

        uint64_t get_bytes_sent() const override { return 0; }
        uint64_t get_bytes_received() const override { return 0; }

        const std::vector<std::string> &hosts() const { return m_hosts; }
        const std::vector<std::string> &ports() const { return m_ports; }
        const std::vector<std::string> &uris() const { return m_uris; }

    private:
        std::vector<FakeResponse> m_responses;
        size_t m_nextResponse = 0;
        epee::net_utils::http::http_response_info m_response{};
        std::vector<std::string> m_hosts;
        std::vector<std::string> m_ports;
        std::vector<std::string> m_uris;
    };
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

    void acceptsOnlyBoundedTrustedHttpsRedirectTargets()
    {
        const QStringList allowedHosts{
            QStringLiteral("github.com"),
            QStringLiteral("release-assets.githubusercontent.com"),
            QStringLiteral("objects.githubusercontent.com")};
        const QUrl githubUrl(QStringLiteral(
            "https://github.com/qwertycoin-org/qwertycoin-gui/releases/download/v2.0.3/asset.dmg"));

        QVERIFY(DownloadRedirectPolicy::isAllowedUrl(githubUrl, allowedHosts));
        QVERIFY(DownloadRedirectPolicy::isAllowedUrl(
            QUrl(QStringLiteral("https://release-assets.githubusercontent.com/github-production-release-asset/1/file")),
            allowedHosts));
        QVERIFY(!DownloadRedirectPolicy::isAllowedUrl(
            QUrl(QStringLiteral("http://release-assets.githubusercontent.com/file")), allowedHosts));
        QVERIFY(!DownloadRedirectPolicy::isAllowedUrl(
            QUrl(QStringLiteral("https://github.com.evil.example/file")), allowedHosts));
        QVERIFY(!DownloadRedirectPolicy::isAllowedUrl(
            QUrl(QStringLiteral("https://user@github.com/file")), allowedHosts));
        QVERIFY(!DownloadRedirectPolicy::isAllowedUrl(
            QUrl(QStringLiteral("https://github.com:8443/file")), allowedHosts));

        QCOMPARE(
            DownloadRedirectPolicy::resolve(
                githubUrl,
                QStringLiteral("https://release-assets.githubusercontent.com/download/file?sig=a%2Bb"),
                allowedHosts).toEncoded(QUrl::FullyEncoded),
            QByteArray("https://release-assets.githubusercontent.com/download/file?sig=a%2Bb"));
        QCOMPARE(
            DownloadRedirectPolicy::resolve(githubUrl, QStringLiteral("../other"), allowedHosts).host(),
            QStringLiteral("github.com"));
        QVERIFY(!DownloadRedirectPolicy::resolve(
            githubUrl, QStringLiteral("https://evil.example/file"), allowedHosts).isValid());
        QVERIFY(!DownloadRedirectPolicy::resolve(
            githubUrl, QStringLiteral("https://release-assets.githubusercontent.com/file\nInjected: yes"),
            allowedHosts).isValid());
    }

    void recognizesOnlyGetCompatibleRedirectStatuses()
    {
        for (const int status : {301, 302, 303, 307, 308})
            QVERIFY(DownloadRedirectPolicy::isRedirectStatus(status));
        for (const int status : {200, 300, 304, 305, 306, 400})
            QVERIFY(!DownloadRedirectPolicy::isRedirectStatus(status));
    }

    void followsTrustedReleaseRedirectAndPreservesEncodedQuery()
    {
        const QStringList allowedHosts{
            QStringLiteral("github.com"),
            QStringLiteral("release-assets.githubusercontent.com")};
        auto client = std::make_shared<FakeHttpClient>(std::vector<FakeResponse>{
            {302, "https://release-assets.githubusercontent.com/download/file?sig=a%2Bb", ""},
            {200, "", "verified payload"}});
        Network network;
        std::string response;

        const QString error = network.get(
            client,
            QStringLiteral("https://github.com/org/repo/releases/download/v2.0.3/file.dmg"),
            response,
            {},
            allowedHosts,
            5);

        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(QString::fromStdString(response), QStringLiteral("verified payload"));
        QCOMPARE(client->hosts().size(), size_t(2));
        QCOMPARE(QString::fromStdString(client->hosts().at(0)), QStringLiteral("github.com"));
        QCOMPARE(QString::fromStdString(client->hosts().at(1)),
            QStringLiteral("release-assets.githubusercontent.com"));
        QCOMPARE(client->ports().size(), size_t(2));
        QCOMPARE(QString::fromStdString(client->ports().at(0)), QStringLiteral("443"));
        QCOMPARE(QString::fromStdString(client->ports().at(1)), QStringLiteral("443"));
        QCOMPARE(client->uris().size(), size_t(2));
        QCOMPARE(QString::fromStdString(client->uris().at(0)),
            QStringLiteral("/org/repo/releases/download/v2.0.3/file.dmg"));
        QCOMPARE(QString::fromStdString(client->uris().at(1)),
            QStringLiteral("/download/file?sig=a%2Bb"));
    }

    void rejectsUntrustedReleaseRedirectBeforeSecondRequest()
    {
        const QStringList allowedHosts{
            QStringLiteral("github.com"),
            QStringLiteral("release-assets.githubusercontent.com")};
        auto client = std::make_shared<FakeHttpClient>(std::vector<FakeResponse>{
            {302, "https://evil.example/file", ""},
            {200, "", "must not be requested"}});
        Network network;
        std::string response;

        const QString error = network.get(
            client,
            QStringLiteral("https://github.com/org/repo/releases/download/v2.0.3/file.dmg"),
            response,
            {},
            allowedHosts,
            5);

        QCOMPARE(error, QStringLiteral("invalid or untrusted redirect"));
        QVERIFY(response.empty());
        QCOMPARE(client->uris().size(), size_t(1));
    }
};

QTEST_GUILESS_MAIN(UpdateMetadataTests)
#include "tst_UpdateMetadata.moc"
