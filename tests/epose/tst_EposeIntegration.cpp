// Copyright (c) 2026, The Qwertycoin Project
// SPDX-License-Identifier: BSD-3-Clause

#include <functional>
#include <limits>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHash>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QSet>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtTest>

#include "daemon/DaemonManager.h"
#include "qt/EposeManager.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/http_abstract_invoke.h"

namespace
{
    QByteArray json(const cryptonote::COMMAND_RPC_GET_EPOSE_INFO::response &response)
    {
        std::string value;
        if (!epee::serialization::store_t_to_json(response, value))
            return {};
        return QByteArray::fromStdString(value);
    }

    template<typename Response>
    QByteArray jsonResponse(const Response &response)
    {
        std::string value;
        if (!epee::serialization::store_t_to_json(response, value))
            return {};
        return QByteArray::fromStdString(value);
    }

    class RpcStub final : public QObject
    {
        Q_OBJECT

    public:
        struct Reply
        {
            Reply(int statusValue = 200, const QByteArray &bodyValue = QByteArray(), int delayValue = 0)
                : status(statusValue), body(bodyValue), delayMs(delayValue) {}

            int status;
            QByteArray body;
            int delayMs;
        };

        RpcStub()
        {
            connect(&m_server, &QTcpServer::newConnection, this, &RpcStub::acceptConnections);
            if (!m_server.listen(QHostAddress::LocalHost, 0))
                qFatal("Unable to start the EPoSe RPC test server");
        }

        QString url() const
        {
            return QStringLiteral("http://127.0.0.1:%1").arg(m_server.serverPort());
        }

        void setReply(const QString &path, const Reply &reply)
        {
            m_replies.insert(path, reply);
        }

        void setDefaultReply(const Reply &reply)
        {
            m_defaultReply = reply;
        }

        QByteArray requestFor(const QString &path) const
        {
            return m_requests.value(path);
        }

        QByteArray replyBodyFor(const QString &path) const
        {
            return m_replies.value(path).body;
        }

        QByteArray deliveredBodyFor(const QString &path) const
        {
            return m_deliveredBodies.value(path);
        }

        QStringList deliveredPaths() const
        {
            return m_deliveredBodies.keys();
        }

    private slots:
        void acceptConnections()
        {
            while (QTcpSocket *socket = m_server.nextPendingConnection())
            {
                connect(socket, &QTcpSocket::readyRead, this,
                        [this, socket] { processSocket(socket); });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        }

    private:
        void processSocket(QTcpSocket *socket)
        {
            m_buffers[socket].append(socket->readAll());
            if (m_waiting.contains(socket))
                return;

            QByteArray &buffer = m_buffers[socket];
            const int headerEnd = buffer.indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return;

            int contentLength = 0;
            const QList<QByteArray> headerLines = buffer.left(headerEnd).split('\n');
            for (QByteArray line : headerLines)
            {
                line = line.trimmed();
                if (line.toLower().startsWith("content-length:"))
                    contentLength = line.mid(line.indexOf(':') + 1).trimmed().toInt();
            }
            const int requestSize = headerEnd + 4 + contentLength;
            if (buffer.size() < requestSize)
                return;

            const QByteArray request = buffer.left(requestSize);
            buffer.remove(0, requestSize);
            const QList<QByteArray> requestLine = request.left(request.indexOf("\r\n")).split(' ');
            const QString path = requestLine.size() >= 2
                ? QString::fromLatin1(requestLine.at(1)) : QString();
            m_requests.insert(path, request);
            const Reply reply = m_replies.value(path, m_defaultReply);
            m_deliveredBodies.insert(path, reply.body);
            m_waiting.insert(socket);
            QPointer<QTcpSocket> guarded(socket);
            QTimer::singleShot(reply.delayMs, this, [this, guarded, reply] {
                if (!guarded)
                    return;
                const QByteArray reason = reply.status == 200 ? QByteArrayLiteral("OK")
                                        : (reply.status == 401 ? QByteArrayLiteral("Unauthorized")
                                                               : QByteArrayLiteral("Not Found"));
                const QByteArray response = QByteArrayLiteral("HTTP/1.1 ")
                    + QByteArray::number(reply.status) + ' ' + reason
                    + QByteArrayLiteral("\r\nContent-Type: application/json\r\nConnection: keep-alive\r\nContent-Length: ")
                    + QByteArray::number(reply.body.size()) + QByteArrayLiteral("\r\n\r\n")
                    + reply.body;
                guarded->write(response);
                guarded->flush();
                m_waiting.remove(guarded.data());
                if (!m_buffers.value(guarded.data()).isEmpty())
                    processSocket(guarded.data());
            });
        }

        QTcpServer m_server;
        QHash<QString, Reply> m_replies;
        Reply m_defaultReply{404, QByteArrayLiteral("{}"), 0};
        QHash<QTcpSocket *, QByteArray> m_buffers;
        QSet<QTcpSocket *> m_waiting;
        QHash<QString, QByteArray> m_requests;
        QHash<QString, QByteArray> m_deliveredBodies;
    };

    bool waitUntil(const std::function<bool()> &condition, const int timeoutMs = 5000)
    {
        QElapsedTimer timer;
        timer.start();
        while (!condition() && timer.elapsed() < timeoutMs)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
            QTest::qWait(10);
        }
        return condition();
    }

    void configureReadyRpc(RpcStub &stub, const int infoDelayMs = 0)
    {
        cryptonote::epose_service_node_entry entry{};
        entry.identity_id = "identity";
        entry.service_public_key = "service-key";
        entry.descriptor_sequence = std::numeric_limits<uint64_t>::max();
        entry.effective_epoch = 7;
        entry.registration_epoch = 6;
        entry.expiry_epoch = 9;
        entry.active = true;
        entry.qualified = true;

        cryptonote::COMMAND_RPC_GET_EPOSE_INFO::response info{};
        info.status = CORE_RPC_STATUS_OK;
        info.enabled = true;
        info.protocol_version = 2;
        info.current_epoch = 7;
        info.epoch_start_height = 5040;
        info.epoch_end_height = 5759;
        info.service_node_count = 2;
        info.qualified_count = 1;
        info.attestation_count = 12;
        info.state_hash = "state-hash";
        info.service_reward_bps = 1000;
        info.local_service_node = true;
        info.local_service_node_key_loaded = true;
        info.local_service_node_registered = true;
        info.local_service_node_active = true;
        info.local_service_node_qualified = true;
        info.local_service_node_expiry_epoch = 9;
        info.local_service_public_key = entry.service_public_key;
        info.local_service_reward_address = "primary-address";
        info.local_service_advertised_endpoint = "service.example.org:8198";
        stub.setReply(QStringLiteral("/get_epose_info"), {200, json(info), infoDelayMs});

        cryptonote::COMMAND_RPC_GET_EPOSE_EPOCH::response epoch{};
        epoch.status = CORE_RPC_STATUS_OK;
        epoch.epoch = 7;
        epoch.start_height = 5040;
        epoch.end_height = 5759;
        epoch.active_count = 2;
        epoch.qualified_count = 1;
        stub.setReply(QStringLiteral("/get_epose_epoch"), {200, jsonResponse(epoch), 0});

        cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response nodes{};
        nodes.status = CORE_RPC_STATUS_OK;
        nodes.service_nodes.push_back(entry);
        nodes.returned_count = 1;
        nodes.total_count = 2;
        stub.setReply(QStringLiteral("/get_service_nodes"), {200, jsonResponse(nodes), 0});

        cryptonote::COMMAND_RPC_GET_SERVICE_NODE_STATUS::response localStatus{};
        localStatus.status = CORE_RPC_STATUS_OK;
        localStatus.found = true;
        localStatus.service_node = entry;
        stub.setReply(QStringLiteral("/get_service_node_status"),
                      {200, jsonResponse(localStatus), 0});

        cryptonote::COMMAND_RPC_GET_SERVICE_REWARDS::response rewards{};
        rewards.status = CORE_RPC_STATUS_OK;
        rewards.preview_available = false;
        rewards.service_reward_active = false;
        rewards.protocol_version = 2;
        rewards.height = 0;
        rewards.epoch = 7;
        rewards.service_reward_bps = 1000;
        rewards.qualified_count = 1;
        stub.setReply(QStringLiteral("/get_service_rewards"), {200, jsonResponse(rewards), 0});

        cryptonote::COMMAND_RPC_GET_SERVICE_NODE_REGISTRATION_PAYLOAD::response legacy{};
        legacy.status = CORE_RPC_STATUS_OK;
        legacy.ready = false;
        legacy.error_details = "Retired: the local producer owns registration.";
        stub.setReply(QStringLiteral("/get_service_node_registration_payload"),
                      {200, jsonResponse(legacy), 0});
    }
}

class EposeIntegrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void readySnapshotPreservesExactValuesAndContractRequests()
    {
        RpcStub stub;
        configureReadyRpc(stub);
        const QByteArray nodesReply = stub.replyBodyFor(QStringLiteral("/get_service_nodes"));
        QVERIFY(nodesReply.contains("identity"));
        cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response roundTripNodes{};
        QVERIFY(epee::serialization::load_t_from_json(roundTripNodes, nodesReply.toStdString()));
        QCOMPARE(roundTripNodes.returned_count, uint64_t(1));
        QCOMPARE(roundTripNodes.service_nodes.size(), size_t(1));
        EposeManager manager;
        manager.configure(stub.url());
        QVERIFY2(waitUntil([&manager] { return !manager.loading() && manager.state() == QStringLiteral("ready"); }),
                 qPrintable(manager.lastError()));

        QCOMPARE(manager.info().value(QStringLiteral("currentEpoch")).toString(), QStringLiteral("7"));
        QCOMPARE(manager.info().value(QStringLiteral("localIdentityId")).toString(), QStringLiteral("identity"));
        QCOMPARE(manager.info().value(QStringLiteral("localDescriptorSequence")).toString(),
                 QStringLiteral("18446744073709551615"));
        QCOMPARE(manager.rewardPreview().value(QStringLiteral("previewAvailable")).toBool(), false);
        QCOMPARE(manager.legacyRegistration().value(QStringLiteral("ready")).toBool(), false);
        QVERIFY2(stub.deliveredBodyFor(QStringLiteral("/get_service_nodes")) == nodesReply,
                 qPrintable(stub.deliveredPaths().join(QStringLiteral(", "))));
        QVERIFY2(manager.serviceNodes().size() == 1,
                 qPrintable(QStringLiteral("nodes=%1 source=%2")
                     .arg(manager.serviceNodes().size())
                     .arg(manager.sourceStatus().value(QStringLiteral("serviceNodes")).toMap()
                              .value(QStringLiteral("details")).toString())));
        QCOMPARE(manager.serviceNodes().first().toMap().value(QStringLiteral("descriptorSequence")).toString(),
                 QStringLiteral("18446744073709551615"));
        QCOMPARE(manager.listTruncated(), true);
        const auto requestObject = [](const QByteArray &request) {
            const int bodyStart = request.indexOf("\r\n\r\n");
            return QJsonDocument::fromJson(request.mid(bodyStart + 4)).object();
        };
        const QJsonObject epochRequest = requestObject(
            stub.requestFor(QStringLiteral("/get_epose_epoch")));
        QVERIFY(!epochRequest.contains(QStringLiteral("epoch"))
                || epochRequest.value(QStringLiteral("epoch")).toInt() == 0);
        const QJsonObject nodesRequest = requestObject(
            stub.requestFor(QStringLiteral("/get_service_nodes")));
        QVERIFY(!nodesRequest.contains(QStringLiteral("limit"))
                || nodesRequest.value(QStringLiteral("limit")).toInt() == 100);
    }

    void distinguishesAccessAndCapabilityFailures()
    {
        RpcStub unauthorized;
        unauthorized.setDefaultReply({401, QByteArrayLiteral("{}"), 0});
        EposeManager manager;
        manager.configure(unauthorized.url(), QStringLiteral("operator"), QStringLiteral("not-logged"));
        QVERIFY(waitUntil([&manager] { return !manager.loading(); }));
        QCOMPARE(manager.state(), QStringLiteral("unauthorized"));
        QVERIFY(manager.lastError().contains(QStringLiteral("access rights")));
        QVERIFY(!manager.lastError().contains(QStringLiteral("not-logged")));
        QVERIFY(!manager.sourceStatus().value(QStringLiteral("eposeInfo")).toMap()
                     .value(QStringLiteral("details")).toString().contains(QStringLiteral("not-logged")));

        RpcStub unsupported;
        unsupported.setDefaultReply({404, QByteArrayLiteral("{}"), 0});
        manager.configure(unsupported.url());
        QVERIFY(waitUntil([&manager] { return !manager.loading(); }));
        QCOMPARE(manager.state(), QStringLiteral("unsupported"));
        QVERIFY(manager.lastError().contains(QStringLiteral("does not support")));
    }

    void malformedAndPartialResponsesAreNotCurrent()
    {
        RpcStub malformed;
        malformed.setReply(QStringLiteral("/get_epose_info"), {200, QByteArrayLiteral("not-json"), 0});
        EposeManager manager;
        manager.configure(malformed.url());
        QVERIFY(waitUntil([&manager] { return !manager.loading(); }));
        QCOMPARE(manager.state(), QStringLiteral("error"));
        QVERIFY(manager.lastError().contains(QStringLiteral("invalid or incomplete")));

        RpcStub partial;
        configureReadyRpc(partial);
        partial.setReply(QStringLiteral("/get_epose_epoch"), {404, QByteArrayLiteral("{}"), 0});
        manager.configure(partial.url());
        QVERIFY(waitUntil([&manager] { return !manager.loading(); }));
        QCOMPARE(manager.state(), QStringLiteral("partial"));
        QCOMPARE(manager.sourceStatus().value(QStringLiteral("epoch")).toMap()
                     .value(QStringLiteral("state")).toString(), QStringLiteral("unsupported"));
    }

    void timesOutAnUnresponsiveDaemon()
    {
        RpcStub slow;
        configureReadyRpc(slow, 9000);
        EposeManager manager;
        manager.configure(slow.url());
        QVERIFY(waitUntil([&manager] { return !manager.loading(); }, 9500));
        QCOMPARE(manager.state(), QStringLiteral("error"));
        QVERIFY(manager.lastError().contains(QStringLiteral("timed out")));
    }

    void staleDaemonResponseIsDiscarded()
    {
        RpcStub slow;
        configureReadyRpc(slow, 250);
        RpcStub current;
        configureReadyRpc(current);

        EposeManager manager;
        manager.configure(slow.url());
        QTest::qWait(30);
        manager.configure(current.url());
        QVERIFY2(waitUntil([&manager, &current] {
            return !manager.loading() && manager.daemonEndpoint() == current.url()
                   && manager.state() == QStringLiteral("ready");
        }), qPrintable(manager.lastError()));
        QCOMPARE(manager.daemonEndpoint(), current.url());
    }

    void validatesLocalProducerConfiguration()
    {
        const QString mainnetAddress = QStringLiteral(
            "QWC1h8SDGxBj74KgQDGKRKAjTfWJywnvM1ZRGv9yX9o89N8qGedHKLheKPmtkLpTmpBWGDT3ZuLUmNVvz3LmU5LrA3gGed58sq");
        DaemonManager manager;
        const QVariantMap valid = manager.validateEposeServiceConfig(
            QStringLiteral("--max-concurrency 2"), NetworkType::MAINNET,
            QStringLiteral("/tmp/qwc-epose-unit"), mainnetAddress,
            QStringLiteral("service.example.org"), 8198,
            QStringLiteral("http://seed-01.example.org:8198 http://seed-02.example.org:8198"));
        QVERIFY2(valid.value(QStringLiteral("valid")).toBool(),
                 qPrintable(valid.value(QStringLiteral("error")).toString()));
        QCOMPARE(valid.value(QStringLiteral("keystorePath")).toString(),
                 QStringLiteral("/tmp/qwc-epose-unit/epose-v2/mainnet/service-keystore-v2"));

        const QStringList arguments = manager.eposeServiceArguments(
            QStringLiteral("--max-concurrency 2"), mainnetAddress,
            QStringLiteral("service.example.org"), 8198, valid);
        QCOMPARE(arguments.count(QStringLiteral("--epose-v2-service")), 1);
        QCOMPARE(arguments.count(QStringLiteral("--confirm-external-bind")), 1);
        QCOMPARE(arguments.value(arguments.indexOf(QStringLiteral("--epose-v2-keystore")) + 1),
                 QStringLiteral("/tmp/qwc-epose-unit/epose-v2/mainnet/service-keystore-v2"));
        QCOMPARE(arguments.value(arguments.indexOf(QStringLiteral("--epose-v2-reward-address")) + 1),
                 mainnetAddress);
        QCOMPARE(arguments.value(arguments.indexOf(QStringLiteral("--rpc-restricted-bind-ip")) + 1),
                 QStringLiteral("0.0.0.0"));
        QCOMPARE(arguments.value(arguments.indexOf(QStringLiteral("--rpc-restricted-bind-port")) + 1),
                 QStringLiteral("8198"));
        QCOMPARE(arguments.count(QStringLiteral("--epose-v2-discovery-endpoint")), 2);

        const QVariantMap rawFlag = manager.validateEposeServiceConfig(
            QStringLiteral("--epose-v2-service"), NetworkType::MAINNET,
            QString(), mainnetAddress, QStringLiteral("service.example.org"), 8198,
            QStringLiteral("http://seed.example.org:8198"));
        QCOMPARE(rawFlag.value(QStringLiteral("valid")).toBool(), false);

        const QVariantMap externalBindOverride = manager.validateEposeServiceConfig(
            QStringLiteral("--confirm-external-bind"), NetworkType::MAINNET,
            QString(), mainnetAddress, QStringLiteral("service.example.org"), 8198,
            QStringLiteral("http://seed.example.org:8198"));
        QCOMPARE(externalBindOverride.value(QStringLiteral("valid")).toBool(), false);

        const QVariantMap wrongNetwork = manager.validateEposeServiceConfig(
            QString(), NetworkType::TESTNET, QString(), mainnetAddress,
            QStringLiteral("service.example.org"), 18198,
            QStringLiteral("http://seed.example.org:18198"));
        QCOMPARE(wrongNetwork.value(QStringLiteral("valid")).toBool(), false);

        const QVariantMap insecureDiscovery = manager.validateEposeServiceConfig(
            QString(), NetworkType::MAINNET, QString(), mainnetAddress,
            QStringLiteral("service.example.org"), 8198,
            QStringLiteral("https://seed.example.org:8198/path"));
        QCOMPARE(insecureDiscovery.value(QStringLiteral("valid")).toBool(), false);

        const QVariantMap privateEndpoint = manager.validateEposeServiceConfig(
            QString(), NetworkType::MAINNET, QString(), mainnetAddress,
            QStringLiteral("127.0.0.1"), 8198,
            QStringLiteral("http://seed.example.org:8198"));
        QCOMPARE(privateEndpoint.value(QStringLiteral("valid")).toBool(), false);

        const QVariantMap nonCanonicalEndpoint = manager.validateEposeServiceConfig(
            QString(), NetworkType::MAINNET, QString(), mainnetAddress,
            QStringLiteral("Service.Example.org"), 8198,
            QStringLiteral("http://seed.example.org:8198"));
        QCOMPARE(nonCanonicalEndpoint.value(QStringLiteral("valid")).toBool(), false);

        const QVariantMap privateDiscovery = manager.validateEposeServiceConfig(
            QString(), NetworkType::MAINNET, QString(), mainnetAddress,
            QStringLiteral("service.example.org"), 8198,
            QStringLiteral("http://192.168.1.20:8198"));
        QCOMPARE(privateDiscovery.value(QStringLiteral("valid")).toBool(), false);
    }
};

QTEST_GUILESS_MAIN(EposeIntegrationTest)

#include "tst_EposeIntegration.moc"
