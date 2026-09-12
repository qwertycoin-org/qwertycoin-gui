// Copyright (c) 2026, The Qwertycoin Project
//
// All rights reserved.

#include "EposeManager.h"

#include <chrono>

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>

#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/http_abstract_invoke.h"

namespace
{
    constexpr std::chrono::milliseconds RPC_TIMEOUT{8000};
    constexpr quint64 SERVICE_NODE_LIMIT = 100;

    QString u64(const uint64_t value)
    {
        return QString::number(static_cast<qulonglong>(value));
    }

    QString text(const std::string &value)
    {
        return QString::fromStdString(value);
    }

    QVariantMap sourceResult(const QString &state, const QString &observedAt,
                             const QString &details = QString(), bool untrusted = false)
    {
        QVariantMap result{{QStringLiteral("state"), state},
                           {QStringLiteral("observedAt"), observedAt},
                           {QStringLiteral("untrusted"), untrusted}};
        if (!details.isEmpty())
            result.insert(QStringLiteral("details"), details);
        return result;
    }

    struct RpcInvocation
    {
        bool timedOut = false;
        bool responseReceived = false;
        bool parsed = false;
        int httpStatus = 0;

        bool ok() const { return responseReceived && httpStatus == 200 && parsed; }
    };

    template<typename Command>
    RpcInvocation invoke(const QUrl &baseUrl,
                         const QString &username,
                         const QString &password,
                         const char *path,
                         typename Command::request &request,
                         typename Command::response &response,
                         QNetworkAccessManager &manager)
    {
        RpcInvocation result;
        std::string requestJson;
        if (!epee::serialization::store_t_to_json(request, requestJson))
            return result;

        QUrl requestUrl(baseUrl);
        QString requestPath = requestUrl.path();
        while (requestPath.endsWith(QLatin1Char('/')))
            requestPath.chop(1);
        requestUrl.setPath(requestPath + QString::fromLatin1(path));
        requestUrl.setQuery(QString());
        requestUrl.setFragment(QString());

        QNetworkRequest networkRequest(requestUrl);
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                                 QStringLiteral("application/json; charset=utf-8"));
        if (!username.isEmpty())
        {
            QByteArray credentials = (username + QLatin1Char(':') + password).toUtf8().toBase64();
            networkRequest.setRawHeader("Authorization", QByteArrayLiteral("Basic ") + credentials);
            credentials.fill('\0');
        }

        QNetworkReply *reply = manager.post(networkRequest, QByteArray::fromStdString(requestJson));
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, [&result, reply] {
            result.timedOut = true;
            reply->abort();
        });
        timer.start(static_cast<int>(RPC_TIMEOUT.count()));
        loop.exec();
        timer.stop();

        result.httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result.responseReceived = result.httpStatus > 0;
        // QNetworkReply is closed by abort(). Avoid reading a timed-out reply:
        // Qt otherwise emits a misleading "device not open" warning even though
        // the timeout was handled deliberately.
        const QByteArray responseBody = result.timedOut ? QByteArray() : reply->readAll();
        if (!result.timedOut && result.httpStatus == 200) {
            const std::string responseJson = responseBody.toStdString();
            result.parsed = epee::serialization::load_t_from_json(
                response, responseJson);
        }
        delete reply;
        return result;
    }

    QString rpcState(const RpcInvocation &invocation)
    {
        if (invocation.httpStatus == 401 || invocation.httpStatus == 403)
            return QStringLiteral("unauthorized");
        if (invocation.httpStatus == 404 || invocation.httpStatus == 405)
            return QStringLiteral("unsupported");
        return QStringLiteral("error");
    }

    QString rpcFailure(const RpcInvocation &invocation, const std::string &status)
    {
        if (invocation.timedOut)
            return QCoreApplication::translate("EposeManager", "The daemon request timed out.");
        if (invocation.httpStatus == 401 || invocation.httpStatus == 403)
            return QCoreApplication::translate("EposeManager", "The daemon rejected the RPC credentials or access rights.");
        if (invocation.httpStatus == 404 || invocation.httpStatus == 405)
            return QCoreApplication::translate("EposeManager", "The connected daemon does not support this EPoSe RPC method.");
        if (!invocation.responseReceived)
            return QCoreApplication::translate("EposeManager", "The daemon did not return an HTTP response.");
        if (invocation.httpStatus != 200)
            return QCoreApplication::translate("EposeManager", "The daemon returned HTTP status %1.").arg(invocation.httpStatus);
        if (!invocation.parsed)
            return QCoreApplication::translate("EposeManager", "The daemon returned an invalid or incomplete EPoSe response.");
        if (status.empty())
            return QCoreApplication::translate("EposeManager", "The daemon response did not include a status.");
        return QCoreApplication::translate("EposeManager", "Daemon status: %1").arg(text(status));
    }

    QVariantMap serviceNode(const cryptonote::epose_service_node_entry &node)
    {
        return {
            {QStringLiteral("identityId"), text(node.identity_id)},
            {QStringLiteral("servicePublicKey"), text(node.service_public_key)},
            {QStringLiteral("operatorAuthorizationPublicKey"), text(node.operator_authorization_public_key)},
            {QStringLiteral("rewardViewPublicKey"), text(node.reward_view_public_key)},
            {QStringLiteral("rewardSpendPublicKey"), text(node.reward_spend_public_key)},
            {QStringLiteral("endpointCommitment"), text(node.endpoint_commitment)},
            {QStringLiteral("admissionHash"), text(node.admission_hash)},
            {QStringLiteral("descriptorSequence"), u64(node.descriptor_sequence)},
            {QStringLiteral("effectiveEpoch"), u64(node.effective_epoch)},
            {QStringLiteral("registrationEpoch"), u64(node.registration_epoch)},
            {QStringLiteral("expiryEpoch"), u64(node.expiry_epoch)},
            {QStringLiteral("active"), node.active},
            {QStringLiteral("qualified"), node.qualified}
        };
    }
}

EposeManager::EposeManager(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<QVariantMap>::finished,
            this, &EposeManager::onRefreshFinished);
}

EposeManager::~EposeManager()
{
    ++m_generation;
    m_watcher.cancel();
    m_watcher.waitForFinished();
    m_password.fill(QChar(0));
}

void EposeManager::configure(const QString &endpoint, const QString &username,
                             const QString &password, const bool localManaged)
{
    QString normalized = endpoint.trimmed();
    if (!normalized.isEmpty() && !normalized.contains(QStringLiteral("://")))
        normalized.prepend(QStringLiteral("http://"));
    while (normalized.endsWith(QLatin1Char('/')))
        normalized.chop(1);

    const bool changed = normalized != m_daemonEndpoint || username != m_username
                         || password != m_password || localManaged != m_localManaged;
    if (!changed)
        return;

    m_password.fill(QChar(0));
    m_daemonEndpoint = normalized;
    m_username = username;
    m_password = password;
    m_localManaged = localManaged;
    ++m_generation;
    m_refreshQueued = m_loading;

    m_state = normalized.isEmpty() ? QStringLiteral("unknown") : QStringLiteral("loading");
    m_lastSuccessfulAt.clear();
    m_lastError.clear();
    m_info.clear();
    m_epoch.clear();
    m_serviceNodes.clear();
    m_rewardPreview.clear();
    m_legacyRegistration.clear();
    m_sourceStatus.clear();
    m_listTruncated = false;
    emit connectionChanged();
    emit snapshotChanged();

    if (!m_loading && !normalized.isEmpty())
        startRefresh();
}

void EposeManager::refresh()
{
    if (m_daemonEndpoint.isEmpty())
        return;
    if (m_loading)
    {
        m_refreshQueued = true;
        return;
    }
    startRefresh();
}

void EposeManager::clear()
{
    configure(QString(), QString(), QString(), false);
}

void EposeManager::setLoading(const bool loading)
{
    if (m_loading == loading)
        return;
    m_loading = loading;
    emit loadingChanged();
}

void EposeManager::startRefresh()
{
    setLoading(true);
    m_state = QStringLiteral("loading");
    emit snapshotChanged();

    const quint64 generation = m_generation;
    const QString endpoint = m_daemonEndpoint;
    const QString username = m_username;
    const QString password = m_password;
    m_watcher.setFuture(QtConcurrent::run(&EposeManager::fetchSnapshot,
                                          endpoint, username, password, generation));
}

void EposeManager::onRefreshFinished()
{
    const QVariantMap snapshot = m_watcher.result();
    setLoading(false);

    if (snapshot.value(QStringLiteral("generation")).toULongLong() == m_generation)
    {
        m_state = snapshot.value(QStringLiteral("state")).toString();
        m_lastSuccessfulAt = snapshot.value(QStringLiteral("lastSuccessfulAt")).toString();
        m_lastError = snapshot.value(QStringLiteral("error")).toString();
        m_info = snapshot.value(QStringLiteral("info")).toMap();
        m_epoch = snapshot.value(QStringLiteral("epoch")).toMap();
        m_serviceNodes = snapshot.value(QStringLiteral("serviceNodes")).toList();
        m_rewardPreview = snapshot.value(QStringLiteral("rewardPreview")).toMap();
        m_legacyRegistration = snapshot.value(QStringLiteral("legacyRegistration")).toMap();
        m_sourceStatus = snapshot.value(QStringLiteral("sourceStatus")).toMap();
        m_listTruncated = snapshot.value(QStringLiteral("listTruncated")).toBool();
        emit snapshotChanged();
    }

    if (m_refreshQueued)
    {
        m_refreshQueued = false;
        if (!m_daemonEndpoint.isEmpty())
            startRefresh();
    }
}

QVariantMap EposeManager::fetchSnapshot(const QString &endpoint, const QString &username,
                                        const QString &password, const quint64 generation)
{
    QVariantMap result{{QStringLiteral("generation"), QVariant::fromValue<qulonglong>(generation)},
                       {QStringLiteral("state"), QStringLiteral("error")}};
    QVariantMap sources;
    const auto observedNow = [] {
        return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    };

    const QUrl baseUrl(endpoint, QUrl::StrictMode);
    if (!baseUrl.isValid() || (baseUrl.scheme() != QStringLiteral("http")
                               && baseUrl.scheme() != QStringLiteral("https"))
        || baseUrl.host().isEmpty() || !baseUrl.userInfo().isEmpty())
    {
        result.insert(QStringLiteral("error"),
                      QCoreApplication::translate("EposeManager", "Invalid daemon endpoint."));
        sources.insert(QStringLiteral("eposeInfo"),
                       sourceResult(QStringLiteral("error"), observedNow(),
                                    QCoreApplication::translate("EposeManager", "Invalid daemon endpoint.")));
        result.insert(QStringLiteral("sourceStatus"), sources);
        return result;
    }

    cryptonote::COMMAND_RPC_GET_EPOSE_INFO::request infoRequest{};
    cryptonote::COMMAND_RPC_GET_EPOSE_INFO::response infoResponse{};
    QNetworkAccessManager manager;
    const RpcInvocation infoInvocation = invoke<cryptonote::COMMAND_RPC_GET_EPOSE_INFO>(
        baseUrl, username, password, "/get_epose_info", infoRequest, infoResponse, manager);
    if (!infoInvocation.ok() || infoResponse.status != CORE_RPC_STATUS_OK)
    {
        const QString failure = rpcFailure(infoInvocation, infoResponse.status);
        result.insert(QStringLiteral("error"), failure);
        result.insert(QStringLiteral("state"), rpcState(infoInvocation));
        sources.insert(QStringLiteral("eposeInfo"),
                       sourceResult(result.value(QStringLiteral("state")).toString(), observedNow(), failure));
        result.insert(QStringLiteral("sourceStatus"), sources);
        return result;
    }

    QVariantMap info{
        {QStringLiteral("enabled"), infoResponse.enabled},
        {QStringLiteral("protocolVersion"), u64(infoResponse.protocol_version)},
        {QStringLiteral("currentEpoch"), u64(infoResponse.current_epoch)},
        {QStringLiteral("epochStartHeight"), u64(infoResponse.epoch_start_height)},
        {QStringLiteral("epochEndHeight"), u64(infoResponse.epoch_end_height)},
        {QStringLiteral("serviceNodeCount"), u64(infoResponse.service_node_count)},
        {QStringLiteral("qualifiedCount"), u64(infoResponse.qualified_count)},
        {QStringLiteral("attestationCount"), u64(infoResponse.attestation_count)},
        {QStringLiteral("stateHash"), text(infoResponse.state_hash)},
        {QStringLiteral("serviceRewardBps"), u64(infoResponse.service_reward_bps)},
        {QStringLiteral("localServiceNode"), infoResponse.local_service_node},
        {QStringLiteral("localKeyLoaded"), infoResponse.local_service_node_key_loaded},
        {QStringLiteral("localRegistered"), infoResponse.local_service_node_registered},
        {QStringLiteral("localActive"), infoResponse.local_service_node_active},
        {QStringLiteral("localQualified"), infoResponse.local_service_node_qualified},
        {QStringLiteral("localExpiryEpoch"), u64(infoResponse.local_service_node_expiry_epoch)},
        {QStringLiteral("localServicePublicKey"), text(infoResponse.local_service_public_key)},
        {QStringLiteral("localRewardAddress"), text(infoResponse.local_service_reward_address)},
        {QStringLiteral("localAdvertisedEndpoint"), text(infoResponse.local_service_advertised_endpoint)},
        {QStringLiteral("localEndpointCommitment"), text(infoResponse.local_service_endpoint_commitment)},
        {QStringLiteral("untrusted"), infoResponse.untrusted}
    };
    result.insert(QStringLiteral("info"), info);
    sources.insert(QStringLiteral("eposeInfo"),
                   sourceResult(QStringLiteral("ready"), observedNow(), QString(), infoResponse.untrusted));

    cryptonote::COMMAND_RPC_GET_EPOSE_EPOCH::request epochRequest{};
    epochRequest.epoch = 0; // Core contract: zero is the current epoch alias.
    cryptonote::COMMAND_RPC_GET_EPOSE_EPOCH::response epochResponse{};
    const RpcInvocation epochInvocation = invoke<cryptonote::COMMAND_RPC_GET_EPOSE_EPOCH>(
        baseUrl, username, password, "/get_epose_epoch", epochRequest, epochResponse, manager);
    if (epochInvocation.ok() && epochResponse.status == CORE_RPC_STATUS_OK)
    {
        result.insert(QStringLiteral("epoch"), QVariantMap{
            {QStringLiteral("epoch"), u64(epochResponse.epoch)},
            {QStringLiteral("startHeight"), u64(epochResponse.start_height)},
            {QStringLiteral("endHeight"), u64(epochResponse.end_height)},
            {QStringLiteral("activeCount"), u64(epochResponse.active_count)},
            {QStringLiteral("qualifiedCount"), u64(epochResponse.qualified_count)},
            {QStringLiteral("untrusted"), epochResponse.untrusted}
        });
        sources.insert(QStringLiteral("epoch"),
                       sourceResult(QStringLiteral("ready"), observedNow(), QString(), epochResponse.untrusted));
    }
    else
    {
        sources.insert(QStringLiteral("epoch"),
                       sourceResult(rpcState(epochInvocation), observedNow(),
                                    rpcFailure(epochInvocation, epochResponse.status)));
    }

    cryptonote::COMMAND_RPC_GET_SERVICE_NODES::request nodesRequest{};
    nodesRequest.limit = SERVICE_NODE_LIMIT;
    cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response nodesResponse{};
    const RpcInvocation nodesInvocation = invoke<cryptonote::COMMAND_RPC_GET_SERVICE_NODES>(
        baseUrl, username, password, "/get_service_nodes", nodesRequest, nodesResponse, manager);
    if (nodesInvocation.ok() && nodesResponse.status == CORE_RPC_STATUS_OK)
    {
        QVariantList nodes;
        nodes.reserve(static_cast<int>(nodesResponse.service_nodes.size()));
        for (const auto &node : nodesResponse.service_nodes)
            nodes.append(serviceNode(node));
        result.insert(QStringLiteral("serviceNodes"), nodes);
        result.insert(QStringLiteral("listTruncated"),
                      nodesResponse.returned_count < nodesResponse.total_count);
        sources.insert(QStringLiteral("serviceNodes"),
                       sourceResult(QStringLiteral("ready"), observedNow(),
                                    QCoreApplication::translate("EposeManager", "Returned %1 of %2 entries.")
                                        .arg(u64(nodesResponse.returned_count), u64(nodesResponse.total_count)),
                                    nodesResponse.untrusted));
    }
    else
    {
        sources.insert(QStringLiteral("serviceNodes"),
                       sourceResult(rpcState(nodesInvocation), observedNow(),
                                    rpcFailure(nodesInvocation, nodesResponse.status)));
    }

    if (!infoResponse.local_service_public_key.empty())
    {
        cryptonote::COMMAND_RPC_GET_SERVICE_NODE_STATUS::request statusRequest{};
        statusRequest.service_public_key = infoResponse.local_service_public_key;
        cryptonote::COMMAND_RPC_GET_SERVICE_NODE_STATUS::response statusResponse{};
        const RpcInvocation statusInvocation = invoke<cryptonote::COMMAND_RPC_GET_SERVICE_NODE_STATUS>(
            baseUrl, username, password, "/get_service_node_status", statusRequest, statusResponse, manager);
        if (statusInvocation.ok() && statusResponse.status == CORE_RPC_STATUS_OK)
        {
            if (statusResponse.found)
            {
                const QVariantMap local = serviceNode(statusResponse.service_node);
                info.insert(QStringLiteral("localIdentityId"), local.value(QStringLiteral("identityId")));
                info.insert(QStringLiteral("localDescriptorSequence"), local.value(QStringLiteral("descriptorSequence")));
                info.insert(QStringLiteral("localEffectiveEpoch"), local.value(QStringLiteral("effectiveEpoch")));
                info.insert(QStringLiteral("localRegistrationEpoch"), local.value(QStringLiteral("registrationEpoch")));
                info.insert(QStringLiteral("localExpiryEpoch"), local.value(QStringLiteral("expiryEpoch")));
                result.insert(QStringLiteral("info"), info);
            }
            sources.insert(QStringLiteral("localServiceNode"),
                           sourceResult(QStringLiteral("ready"), observedNow(),
                                        statusResponse.found ? QString() : QCoreApplication::translate(
                                            "EposeManager", "The current service key is not in the effective registry."),
                                        statusResponse.untrusted));
        }
        else
        {
            sources.insert(QStringLiteral("localServiceNode"),
                           sourceResult(rpcState(statusInvocation), observedNow(),
                                        rpcFailure(statusInvocation, statusResponse.status)));
        }
    }

    cryptonote::COMMAND_RPC_GET_SERVICE_REWARDS::request rewardsRequest{};
    rewardsRequest.height = 0;
    cryptonote::COMMAND_RPC_GET_SERVICE_REWARDS::response rewardsResponse{};
    const RpcInvocation rewardsInvocation = invoke<cryptonote::COMMAND_RPC_GET_SERVICE_REWARDS>(
        baseUrl, username, password, "/get_service_rewards", rewardsRequest, rewardsResponse, manager);
    if (rewardsInvocation.ok() && rewardsResponse.status == CORE_RPC_STATUS_OK)
    {
        result.insert(QStringLiteral("rewardPreview"), QVariantMap{
            {QStringLiteral("previewAvailable"), rewardsResponse.preview_available},
            {QStringLiteral("serviceRewardActive"), rewardsResponse.service_reward_active},
            {QStringLiteral("protocolVersion"), u64(rewardsResponse.protocol_version)},
            {QStringLiteral("height"), u64(rewardsResponse.height)},
            {QStringLiteral("epoch"), u64(rewardsResponse.epoch)},
            {QStringLiteral("serviceRewardBps"), u64(rewardsResponse.service_reward_bps)},
            {QStringLiteral("qualifiedCount"), u64(rewardsResponse.qualified_count)},
            {QStringLiteral("expectedPayeeServicePublicKey"), text(rewardsResponse.expected_payee_service_public_key)},
            {QStringLiteral("expectedRewardViewPublicKey"), text(rewardsResponse.expected_reward_view_public_key)},
            {QStringLiteral("expectedRewardSpendPublicKey"), text(rewardsResponse.expected_reward_spend_public_key)},
            {QStringLiteral("untrusted"), rewardsResponse.untrusted}
        });
        sources.insert(QStringLiteral("rewardPreview"),
                       sourceResult(QStringLiteral("ready"), observedNow(), QString(), rewardsResponse.untrusted));
    }
    else
    {
        sources.insert(QStringLiteral("rewardPreview"),
                       sourceResult(rpcState(rewardsInvocation), observedNow(),
                                    rpcFailure(rewardsInvocation, rewardsResponse.status)));
    }

    cryptonote::COMMAND_RPC_GET_SERVICE_NODE_REGISTRATION_PAYLOAD::request legacyRequest{};
    cryptonote::COMMAND_RPC_GET_SERVICE_NODE_REGISTRATION_PAYLOAD::response legacyResponse{};
    const RpcInvocation legacyInvocation = invoke<cryptonote::COMMAND_RPC_GET_SERVICE_NODE_REGISTRATION_PAYLOAD>(
        baseUrl, username, password, "/get_service_node_registration_payload", legacyRequest, legacyResponse, manager);
    if (legacyInvocation.ok() && legacyResponse.status == CORE_RPC_STATUS_OK)
    {
        result.insert(QStringLiteral("legacyRegistration"), QVariantMap{
            {QStringLiteral("ready"), legacyResponse.ready},
            {QStringLiteral("details"), text(legacyResponse.error_details)},
            {QStringLiteral("retired"), !legacyResponse.ready},
            {QStringLiteral("untrusted"), legacyResponse.untrusted}
        });
        sources.insert(QStringLiteral("legacyRegistration"),
                       sourceResult(QStringLiteral("ready"), observedNow(), QString(), legacyResponse.untrusted));
    }
    else
    {
        sources.insert(QStringLiteral("legacyRegistration"),
                       sourceResult(rpcState(legacyInvocation), observedNow(),
                                    rpcFailure(legacyInvocation, legacyResponse.status)));
    }

    result.insert(QStringLiteral("sourceStatus"), sources);
    result.insert(QStringLiteral("lastSuccessfulAt"), observedNow());
    const bool partial = sources.value(QStringLiteral("epoch")).toMap().value(QStringLiteral("state")) != QStringLiteral("ready")
                         || sources.value(QStringLiteral("serviceNodes")).toMap().value(QStringLiteral("state")) != QStringLiteral("ready")
                         || sources.value(QStringLiteral("rewardPreview")).toMap().value(QStringLiteral("state")) != QStringLiteral("ready")
                         || (!infoResponse.local_service_public_key.empty()
                             && sources.value(QStringLiteral("localServiceNode")).toMap()
                                    .value(QStringLiteral("state")) != QStringLiteral("ready"));
    result.insert(QStringLiteral("state"), partial ? QStringLiteral("partial") : QStringLiteral("ready"));
    return result;
}
