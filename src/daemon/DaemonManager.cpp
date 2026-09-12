// Copyright (c) 2014-2024, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "DaemonManager.h"
#include "common/util.h"
#include "epose/resource_policy_v2.h"
#include "epose/service_node_config.h"
#include <QElapsedTimer>
#include <QFile>
#include <QMutexLocker>
#include <QThread>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>
#include <QApplication>
#include <QProcess>
#include <QStorageInfo>
#include <QVariantMap>
#include <QVariant>
#include <QMap>
#include <QRegularExpression>

#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/address.hpp>

namespace {
    static const int DAEMON_START_TIMEOUT_SECONDS = 120;

    QStringList splitDaemonFlags(const QString &flags)
    {
        QStringList result;
        QString token;
        QChar quote;
        bool escaped = false;
        for (const QChar character : flags)
        {
            if (escaped)
            {
                token.append(character);
                escaped = false;
            }
            else if (character == QLatin1Char('\\'))
            {
                escaped = true;
            }
            else if (!quote.isNull())
            {
                if (character == quote)
                    quote = QChar();
                else
                    token.append(character);
            }
            else if (character == QLatin1Char('\'') || character == QLatin1Char('"'))
            {
                quote = character;
            }
            else if (character.isSpace())
            {
                if (!token.isEmpty())
                {
                    result.append(token);
                    token.clear();
                }
            }
            else
            {
                token.append(character);
            }
        }
        if (escaped)
            token.append(QLatin1Char('\\'));
        if (!token.isEmpty())
            result.append(token);
        return result;
    }

    bool canonicalEposeDnsName(const QString &host)
    {
        const QByteArray bytes = host.toLatin1();
        if (host.isEmpty() || bytes.size() > 253 || host.startsWith(QLatin1Char('.'))
            || host.endsWith(QLatin1Char('.')) || !host.contains(QLatin1Char('.')))
        {
            return false;
        }

        const QStringList labels = host.split(QLatin1Char('.'));
        for (const QString &label : labels)
        {
            if (label.isEmpty() || label.size() > 63 || label.startsWith(QLatin1Char('-'))
                || label.endsWith(QLatin1Char('-')))
            {
                return false;
            }
            for (const QChar character : label)
            {
                const ushort code = character.unicode();
                if (!((code >= 'a' && code <= 'z') || (code >= '0' && code <= '9')
                      || code == '-'))
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool canonicalPublicEposeHost(const QString &host)
    {
        boost::system::error_code error;
        const auto address = boost::asio::ip::make_address(host.toStdString(), error);
        if (!error)
        {
            return QString::fromStdString(address.to_string()) == host
                && qwertycoin::epose::public_probe_address_v2(host.toStdString());
        }
        return canonicalEposeDnsName(host);
    }
}

bool DaemonManager::start(const QString &flags, NetworkType::Type nettype, const QString &dataDir, const QString &bootstrapNodeAddress, bool noSync /* = false*/, bool pruneBlockchain /* = false*/)
{
    return startWithArguments(splitDaemonFlags(flags), nettype, dataDir,
                              bootstrapNodeAddress, noSync, pruneBlockchain);
}

bool DaemonManager::startWithArguments(const QStringList &customArguments,
                                       NetworkType::Type nettype,
                                       const QString &dataDir,
                                       const QString &bootstrapNodeAddress,
                                       bool noSync,
                                       bool pruneBlockchain)
{
    if (!QFileInfo(m_qwertycoind).isFile())
    {
        emit daemonStartFailure("\"" + QDir::toNativeSeparators(m_qwertycoind) + "\" " + tr("executable is missing"));
        return false;
    }

    // prepare command line arguments and pass to qwertycoind
    QStringList arguments;

    // Start daemon with --detach flag on non-windows platforms
#ifndef Q_OS_WIN
    arguments << "--detach";
#endif

    if (nettype == NetworkType::TESTNET)
        arguments << "--testnet";
    else if (nettype == NetworkType::STAGENET)
        arguments << "--stagenet";

    // Custom startup flags for daemon
    arguments << customArguments;

    // Custom data-dir
    if(!dataDir.isEmpty()) {
        arguments << "--data-dir" << dataDir;
    }

    // Bootstrap node address
    if(!bootstrapNodeAddress.isEmpty()) {
        arguments << "--bootstrap-daemon-address" << bootstrapNodeAddress;
    }

    if (pruneBlockchain) {
        if (!checkLmdbExists(dataDir)) { // check that DB has not already been created
            arguments << "--prune-blockchain";
        }
    }

    if (noSync) {
        arguments << "--no-sync";
    }

    arguments << "--check-updates" << "disabled";
    arguments << "--non-interactive";

    // --max-concurrency based on threads available.
    int32_t concurrency = qMax(1, QThread::idealThreadCount() / 2);

    if(!customArguments.contains("--max-concurrency", Qt::CaseSensitive)){
        arguments << "--max-concurrency" << QString::number(concurrency);
    }

    qDebug() << "starting qwertycoind " + m_qwertycoind;

    QMutexLocker locker(&m_daemonMutex);

    m_daemon.reset(new QProcess());

    // Connect output slots
    connect(m_daemon.get(), SIGNAL(readyReadStandardOutput()), this, SLOT(printOutput()));
    connect(m_daemon.get(), SIGNAL(readyReadStandardError()), this, SLOT(printError()));

    // Start qwertycoind
    bool started = m_daemon->startDetached(m_qwertycoind, arguments);

    // add state changed listener
    connect(m_daemon.get(), SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(stateChanged(QProcess::ProcessState)));

    if (!started) {
        qDebug() << "Daemon start error: " + m_daemon->errorString();
        emit daemonStartFailure(m_daemon->errorString());
        return false;
    }

    // Start start watcher
    m_scheduler.run([this, nettype, dataDir, noSync] {
        if (startWatcher(nettype, dataDir)) {
            emit daemonStarted();
            m_noSync = noSync;
        } else {
            emit daemonStartFailure(tr("Timed out, local node is not responding after %1 seconds").arg(DAEMON_START_TIMEOUT_SECONDS));
        }
    });

    return true;
}

QString DaemonManager::eposeKeystorePath(const QString &dataDir, const NetworkType::Type nettype) const
{
    const QString base = dataDir.trimmed().isEmpty()
        ? QString::fromStdString(tools::get_default_data_dir())
        : QDir::cleanPath(dataDir.trimmed());
    const QString network = nettype == NetworkType::TESTNET ? QStringLiteral("testnet")
                          : (nettype == NetworkType::STAGENET ? QStringLiteral("stagenet")
                                                              : QStringLiteral("mainnet"));
    return QDir(base).filePath(QStringLiteral("epose-v2/%1/service-keystore-v2").arg(network));
}

QVariantMap DaemonManager::validateEposeServiceConfig(
    const QString &flags, const NetworkType::Type nettype, const QString &dataDir,
    const QString &rewardAddress, const QString &endpointHost, const int endpointPort,
    const QString &discoveryEndpoints) const
{
    QVariantMap result{{QStringLiteral("valid"), false},
                       {QStringLiteral("keystorePath"), eposeKeystorePath(dataDir, nettype)}};

    const QStringList rawArguments = splitDaemonFlags(flags);
    static const QStringList forbiddenPrefixes{
        QStringLiteral("--epose-v2-"),
        QStringLiteral("--service-node"),
        QStringLiteral("--service-reward"),
        QStringLiteral("--rpc-restricted-bind"),
        QStringLiteral("--restricted-rpc"),
        QStringLiteral("--confirm-external-bind")
    };
    for (const QString &argument : rawArguments)
    {
        for (const QString &prefix : forbiddenPrefixes)
        {
            if (argument.startsWith(prefix, Qt::CaseSensitive))
            {
                result.insert(QStringLiteral("error"),
                              tr("Remove %1 from custom daemon flags. EPoSe manages this option.")
                                  .arg(argument.section(QLatin1Char('='), 0, 0)));
                return result;
            }
        }
    }

    cryptonote::network_type coreNettype = cryptonote::MAINNET;
    if (nettype == NetworkType::TESTNET)
        coreNettype = cryptonote::TESTNET;
    else if (nettype == NetworkType::STAGENET)
        coreNettype = cryptonote::STAGENET;

    cryptonote::account_public_address parsedAddress{};
    std::string addressError;
    if (!qwertycoin::epose::parse_reward_address(
            rewardAddress.trimmed().toStdString(), coreNettype, parsedAddress, addressError))
    {
        result.insert(QStringLiteral("error"),
                      tr("Invalid primary reward address for the selected network: %1")
                          .arg(QString::fromStdString(addressError)));
        return result;
    }

    const QString host = endpointHost.trimmed();
    if (!canonicalPublicEposeHost(host))
    {
        result.insert(QStringLiteral("error"),
                      tr("Endpoint host must be a canonical public IP address or lowercase DNS name."));
        return result;
    }
    if (endpointPort < 1 || endpointPort > 65535)
    {
        result.insert(QStringLiteral("error"), tr("Endpoint port must be between 1 and 65535."));
        return result;
    }
    if (endpointPort == cryptonote::get_config(coreNettype).RPC_DEFAULT_PORT)
    {
        result.insert(QStringLiteral("error"),
                      tr("The restricted EPoSe endpoint port must differ from the local administrative RPC port (%1).")
                          .arg(cryptonote::get_config(coreNettype).RPC_DEFAULT_PORT));
        return result;
    }

    QStringList discoveryUrls;
    const QStringList candidates = discoveryEndpoints.split(
        QRegularExpression(QStringLiteral("[\\s,;]+")), Qt::SkipEmptyParts);
    for (const QString &candidate : candidates)
    {
        const QUrl url(candidate, QUrl::StrictMode);
        if (!url.isValid() || url.scheme() != QStringLiteral("http")
            || !canonicalPublicEposeHost(url.host())
            || url.port() < 1 || url.port() > 65535
            || (!url.path().isEmpty() && url.path() != QStringLiteral("/"))
            || !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment())
        {
            result.insert(QStringLiteral("error"),
                          tr("Discovery endpoint is invalid: %1. Use http://host:port only.")
                              .arg(candidate));
            return result;
        }
        discoveryUrls.append(url.toString(QUrl::RemovePath | QUrl::StripTrailingSlash));
    }
    if (discoveryUrls.isEmpty())
    {
        result.insert(QStringLiteral("error"),
                      tr("At least one EPoSe discovery endpoint is required."));
        return result;
    }

    result.insert(QStringLiteral("valid"), true);
    result.insert(QStringLiteral("error"), QString());
    result.insert(QStringLiteral("endpoint"), QStringLiteral("%1:%2").arg(host).arg(endpointPort));
    result.insert(QStringLiteral("discoveryEndpoints"), discoveryUrls);
    result.insert(QStringLiteral("keystoreExists"), QFileInfo::exists(eposeKeystorePath(dataDir, nettype)));
    return result;
}

bool DaemonManager::startEposeService(
    const QString &flags, const NetworkType::Type nettype, const QString &dataDir,
    const QString &bootstrapNodeAddress, const QString &rewardAddress,
    const QString &endpointHost, const int endpointPort,
    const QString &discoveryEndpoints, const bool noSync, const bool pruneBlockchain)
{
    const QVariantMap validation = validateEposeServiceConfig(
        flags, nettype, dataDir, rewardAddress, endpointHost, endpointPort, discoveryEndpoints);
    if (!validation.value(QStringLiteral("valid")).toBool())
    {
        emit daemonStartFailure(validation.value(QStringLiteral("error")).toString());
        return false;
    }

    const QStringList arguments = eposeServiceArguments(
        flags, rewardAddress, endpointHost, endpointPort, validation);

    return startWithArguments(arguments, nettype, dataDir, bootstrapNodeAddress,
                              noSync, pruneBlockchain);
}

QStringList DaemonManager::eposeServiceArguments(
    const QString &flags, const QString &rewardAddress,
    const QString &endpointHost, const int endpointPort,
    const QVariantMap &validation) const
{
    QStringList arguments = splitDaemonFlags(flags);
    arguments << QStringLiteral("--epose-v2-service")
              << QStringLiteral("--epose-v2-keystore")
              << validation.value(QStringLiteral("keystorePath")).toString()
              << QStringLiteral("--epose-v2-reward-address")
              << rewardAddress.trimmed()
              << QStringLiteral("--epose-v2-endpoint-host")
              << endpointHost.trimmed()
              << QStringLiteral("--epose-v2-endpoint-port")
              << QString::number(endpointPort)
              << QStringLiteral("--rpc-restricted-bind-port")
              << QString::number(endpointPort)
              << QStringLiteral("--rpc-restricted-bind-ip")
              << QStringLiteral("0.0.0.0")
              << QStringLiteral("--confirm-external-bind");
    if (endpointHost.contains(QLatin1Char(':')))
        arguments << QStringLiteral("--rpc-use-ipv6")
                  << QStringLiteral("--rpc-restricted-bind-ipv6-address")
                  << QStringLiteral("::");
    const QStringList discoveryUrls = validation.value(QStringLiteral("discoveryEndpoints")).toStringList();
    for (const QString &url : discoveryUrls)
        arguments << QStringLiteral("--epose-v2-discovery-endpoint") << url;
    return arguments;
}

void DaemonManager::stopAsync(NetworkType::Type nettype, const QString &dataDir, const QJSValue& callback)
{
    const auto feature = m_scheduler.run([this, nettype, dataDir] {
        QString message;
        sendCommand({"exit"}, nettype, dataDir, message);

        return QJSValueList{QJSValue(stopWatcher(nettype, dataDir))};
    }, callback);

    if (!feature.first)
    {
        QJSValue(callback).call(QJSValueList{QJSValue(false)});
    }
}

bool DaemonManager::startWatcher(NetworkType::Type nettype, const QString &dataDir) const
{
    // Check if daemon is started every 2 seconds
    QElapsedTimer timer;
    timer.start();
    while(true && !m_app_exit && timer.elapsed() / 1000 < DAEMON_START_TIMEOUT_SECONDS  ) {
        QThread::sleep(2);
        if(!running(nettype, dataDir)) {
            qDebug() << "daemon not running. checking again in 2 seconds.";
        } else {
            qDebug() << "daemon is started. Waiting 5 seconds to let daemon catch up";
            QThread::sleep(5);
            return true;
        }
    }
    return false;
}

bool DaemonManager::stopWatcher(NetworkType::Type nettype, const QString &dataDir) const
{
    // Check if daemon is running every 2 seconds. Kill if still running after 10 seconds
    int counter = 0;
    while(true && !m_app_exit) {
        QThread::sleep(2);
        counter++;
        if(running(nettype, dataDir)) {
            qDebug() << "Daemon still running.  " << counter;
            if(counter >= 5) {
                qDebug() << "Killing it! ";
#ifdef Q_OS_WIN
                QProcess::execute("taskkill",  {"/F", "/IM", "qwertycoind.exe"});
#else
                QProcess::execute("pkill", {"qwertycoind"});
#endif
            }

        } else
            return true;
    }
    return false;
}


void DaemonManager::stateChanged(QProcess::ProcessState state)
{
    qDebug() << "STATE CHANGED: " << state;
    if (state == QProcess::NotRunning) {
        emit daemonStopped();
    }
}

void DaemonManager::printOutput()
{
    QByteArray byteArray = [this]() {
        QMutexLocker locker(&m_daemonMutex);
        return m_daemon->readAllStandardOutput();
    }();
    QStringList strLines = QString(byteArray).split("\n");

    foreach (QString line, strLines) {
        emit daemonConsoleUpdated(line);
        qDebug() << "Daemon: " + line;
    }
}

void DaemonManager::printError()
{
    QByteArray byteArray = [this]() {
        QMutexLocker locker(&m_daemonMutex);
        return m_daemon->readAllStandardError();
    }();
    QStringList strLines = QString(byteArray).split("\n");

    foreach (QString line, strLines) {
        emit daemonConsoleUpdated(line);
        qDebug() << "Daemon ERROR: " + line;
    }
}

bool DaemonManager::running(NetworkType::Type nettype, const QString &dataDir) const
{
    QString status;
    sendCommand({"sync_info"}, nettype, dataDir, status);
    qDebug() << status;
    return status.contains("Height:");
}

bool DaemonManager::noSync() const noexcept
{
    return m_noSync;
}

void DaemonManager::runningAsync(NetworkType::Type nettype, const QString &dataDir, const QJSValue& callback) const
{
    m_scheduler.run([this, nettype, dataDir] {
        return QJSValueList{QJSValue(running(nettype, dataDir))};
    }, callback);
}

bool DaemonManager::sendCommand(const QStringList &cmd, NetworkType::Type nettype, const QString &dataDir, QString &message) const
{
    QProcess p;
    QStringList external_cmd(cmd);

    // Add network type flag if needed
    if (nettype == NetworkType::TESTNET)
        external_cmd << "--testnet";
    else if (nettype == NetworkType::STAGENET)
        external_cmd << "--stagenet";

    // Custom data-dir
    if (!dataDir.isEmpty()) {
        external_cmd << "--data-dir" << dataDir;
    }

    qDebug() << "sending external cmd: " << external_cmd;


    p.start(m_qwertycoind, external_cmd);

    bool started = p.waitForFinished(-1);
    message = p.readAllStandardOutput();
    emit daemonConsoleUpdated(message);
    return started;
}

void DaemonManager::sendCommandAsync(const QStringList &cmd, NetworkType::Type nettype, const QString &dataDir, const QJSValue& callback) const
{
    m_scheduler.run([this, cmd, nettype, dataDir] {
        QString message;
        return QJSValueList{QJSValue(sendCommand(cmd, nettype, dataDir, message))};
    }, callback);
}

void DaemonManager::exit()
{
    qDebug("DaemonManager: exit()");
    m_app_exit = true;
}

QVariantMap DaemonManager::validateDataDir(const QString &dataDir, const int estimatedBlockchainSize) const
{
    QVariantMap result;
    bool valid = true;
    bool readOnly = false;
    int  storageAvailable = 0;
    bool lmdbExists = true;

    QStorageInfo storage(dataDir);
    if (storage.isValid() && storage.isReady()) {
        if (storage.isReadOnly()) {
            readOnly = true;
            valid = false;
        }

        storageAvailable = storage.bytesAvailable()/1000/1000/1000;
        if (storageAvailable < estimatedBlockchainSize) {
            valid = false;
        }
    } else {
        valid = false;
    }


    if (!QDir(dataDir+"/lmdb").exists()) {
        lmdbExists = false;
        valid = false;
    }

    result.insert("valid", valid);
    result.insert("lmdbExists", lmdbExists);
    result.insert("readOnly", readOnly);
    result.insert("storageAvailable", storageAvailable);

    return result;
}

bool DaemonManager::checkLmdbExists(QString datadir) {
    if (datadir.isEmpty() || datadir.isNull()) {
        datadir = QString::fromStdString(tools::get_default_data_dir());
    }
    return QDir(datadir + "/lmdb").exists();
}

QString DaemonManager::getArgs(const QString &dataDir) {
    if (!running(NetworkType::MAINNET, dataDir)) {
        return args;
    }
    QProcess p;
    QStringList tempArgs;
    #ifdef Q_OS_WIN
        //powershell
        tempArgs << "Get-CimInstance Win32_Process -Filter \"name = 'qwertycoind.exe'\" | select -ExpandProperty CommandLine ";
        p.setProgram("powershell");
        p.setArguments(tempArgs);
        p.start();
        p.waitForFinished();
        args = p.readAllStandardOutput().simplified().trimmed();

    #elif defined(Q_OS_UNIX)
        //pgrep
        tempArgs << "qwertycoind";
        p.setProgram("pgrep");
        p.setArguments(tempArgs);
        p.start();
        p.waitForFinished();
        QString pid = p.readAllStandardOutput().trimmed();
        if (pid.isEmpty()) {
            return args;
        }

        tempArgs.clear();

        //ps
        tempArgs << "-o";
        tempArgs << "args=";
        tempArgs << "-fp";
        tempArgs << pid;
        p.setProgram("ps");
        p.setArguments(tempArgs);
        p.start();
        p.waitForFinished();
        args = p.readAllStandardOutput().trimmed();

    #endif
    if (args.contains("--")) {
        int index = args.indexOf("--");
        args.remove(0, index);
    }
    else {
        args = "";
    }
    return args;
}

DaemonManager::DaemonManager(QObject *parent)
    : QObject(parent)
    , m_scheduler(this)
{

    // Platform depetent path to qwertycoind
#ifdef Q_OS_WIN
    m_qwertycoind = QApplication::applicationDirPath() + "/qwertycoind.exe";
#elif defined(Q_OS_UNIX)
    m_qwertycoind = QApplication::applicationDirPath() + "/qwertycoind";
#endif

    if (m_qwertycoind.length() == 0) {
        qCritical() << "no daemon binary defined for current platform";
    }
}

DaemonManager::~DaemonManager()
{
    m_scheduler.shutdownWaitForFinished();
}
