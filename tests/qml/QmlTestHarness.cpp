// Copyright (c) 2026, The Monero Project
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

#include "QmlTestHarness.h"

#include <QDir>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QVector>
#include <QtQml>
#include <QtQuickTest/quicktest.h>

#include "TranslationManager.h"
#include "libwalletqt/Wallet.h"
#include "libwalletqt/WalletManager.h"
#include "libwalletqt/Messenger.h"
#include "main/clipboardAdapter.h"
#include "main/oshelper.h"
#include "qt/KeysFiles.h"
#include "version.h"

// Exercise the actual linked wallet/bridge, not the page's QML mock. Every
// wallet is newly generated, unfunded, temporary and never connected to a node.
class MessengerTestAccess : public QObject
{
    Q_OBJECT

    struct TestWallet {
        WalletManager manager;
        Wallet *value;
        TestWallet(const QString &path, const QString &password, bool reopen = false)
            : value(reopen ? manager.openWallet(path, password)
                           : manager.createWallet(path, password, QStringLiteral("English"))) {}
        ~TestWallet() { manager.closeWallet(); }
    };

    // A non-broadcasting disposal sentinel. Its destructor must run while the
    // real Core wallet is still alive, exactly as a prepared plan requires.
    struct PreparedPlan : Monero::PendingTransaction {
        Monero::Wallet *wallet;
        bool &disposed;
        PreparedPlan(Monero::Wallet *wallet, bool &disposed) : wallet(wallet), disposed(disposed) {}
        ~PreparedPlan() override { disposed = wallet->status() == Monero::Wallet::Status_Ok; }
        int status() const override { return Status_Ok; }
        std::string errorString() const override { return {}; }
        bool commit(const std::string &, bool) override { return false; }
        uint64_t amount() const override { return 0; }
        uint64_t dust() const override { return 0; }
        uint64_t fee() const override { return 0; }
        std::vector<std::string> txid() const override { return {}; }
        uint64_t txCount() const override { return 1; }
        std::vector<uint32_t> subaddrAccount() const override { return {}; }
        std::vector<std::set<uint32_t>> subaddrIndices() const override { return {}; }
        std::string qmsJournalData() const override { return {}; }
        std::string multisigSignData() override { return {}; }
        void signMultisigTx() override {}
        std::vector<std::string> signersKeys() const override { return {}; }
    };

public:
    Q_INVOKABLE bool passwordGate()
    {
        QTemporaryDir dir;
        if (!dir.isValid()) return false;
        TestWallet wallet(dir.filePath(QStringLiteral("wallet")), QString());
        Messenger *messenger = wallet.value->messenger();
        return wallet.value->status() == Wallet::Status_Ok && !messenger->available()
            && messenger->ownInvitation().isEmpty()
            && !messenger->importInvitation(QStringLiteral("blocked"), QStringLiteral("00"));
    }

    Q_INVOKABLE bool encryptedStateReopens()
    {
        QTemporaryDir dir;
        if (!dir.isValid()) return false;
        const QString path = dir.filePath(QStringLiteral("wallet"));
        const QString password = QStringLiteral("disposable-messenger-test");
        QString ownInvitation;
        QString fingerprint;
        {
            TestWallet wallet(path, password);
            Messenger *messenger = wallet.value->messenger();
            if (wallet.value->status() != Wallet::Status_Ok || !messenger->available()) return false;
            ownInvitation = messenger->ownInvitation();
            const auto contact = qwertycoin::qms::create_invitation(
                qwertycoin::qms::generate_identity(), messenger->genesis());
            const QString invitation = QString::fromStdString(
                qwertycoin::qms::hex(qwertycoin::qms::encode_invitation(contact)));
            const auto fp = qwertycoin::qms::fingerprint(contact.box_public, contact.sign_public);
            fingerprint = Messenger::hex(fp.data(), fp.size());
            if (ownInvitation.isEmpty() || !messenger->importInvitation(QStringLiteral("Test contact"), invitation)
                || messenger->contacts().at(0).toMap().value("confirmed").toBool()
                || messenger->confirmContact(fingerprint, QString(64, QLatin1Char('0')))
                || !messenger->confirmContact(fingerprint, fingerprint)) return false;
        }
        TestWallet reopened(path, password, true);
        Messenger *messenger = reopened.value->messenger();
        return reopened.value->status() == Wallet::Status_Ok
            && messenger->ownInvitation() == ownInvitation && messenger->contacts().size() == 1
            && messenger->contacts().at(0).toMap().value("fingerprint").toString() == fingerprint
            && messenger->contacts().at(0).toMap().value("confirmed").toBool();
    }

    Q_INVOKABLE bool closeDisposesPreparedPlan()
    {
        QTemporaryDir dir;
        if (!dir.isValid()) return false;
        bool disposed = false;
        {
            TestWallet wallet(dir.filePath(QStringLiteral("wallet")), QStringLiteral("disposable-test"));
            Messenger *messenger = wallet.value->messenger();
            if (wallet.value->status() != Wallet::Status_Ok) return false;
            messenger->m_prepared = new PreparedPlan(messenger->m_wallet, disposed);
        }
        return disposed;
    }
};

class QmlTestSetup : public QObject
{
    Q_OBJECT

public:
    QmlTestSetup() : m_accountsDir(QDir::tempPath() + QStringLiteral("/qwertycoin-gui-qml-test-XXXXXX")) {}

public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        qmlRegisterType<clipboardAdapter>("moneroComponents.Clipboard", 1, 0, "Clipboard");
        qmlRegisterType<WalletKeysFilesModel>("moneroComponents.WalletKeysFilesModel", 1, 0, "WalletKeysFilesModel");
        qmlRegisterType<WalletManager>("moneroComponents.WalletManager", 1, 0, "WalletManager");
        qmlRegisterUncreatableType<Wallet>("moneroComponents.Wallet", 1, 0, "Wallet", "Wallet can't be instantiated directly");
        qmlRegisterUncreatableType<Messenger>("moneroComponents.Messenger", 1, 0, "Messenger", "Messenger can't be instantiated directly");
        qmlRegisterType<NetworkType>("moneroComponents.NetworkType", 1, 0, "NetworkType");

        engine->addImportPath(QStringLiteral(":/fonts"));
        engine->rootContext()->setContextProperty(QStringLiteral("translationManager"), TranslationManager::instance());
        engine->rootContext()->setContextProperty(QStringLiteral("oshelper"), &m_osHelper);
        engine->rootContext()->setContextProperty(QStringLiteral("nativeMessengerTests"), &m_messengerTests);
        engine->rootContext()->setContextProperty(
            QStringLiteral("moneroAccountsDir"),
            QDir(m_accountsDir.path()).filePath(QStringLiteral("Qwertycoin/wallets")));
        engine->rootContext()->setContextProperty(QStringLiteral("defaultAccountName"), QStringLiteral("qml-test-wallet"));
        engine->rootContext()->setContextProperty(QStringLiteral("isAndroid"), false);
        engine->rootContext()->setContextProperty(QStringLiteral("isIOS"), false);
        engine->rootContext()->setContextProperty(QStringLiteral("isLinux"), false);
        engine->rootContext()->setContextProperty(QStringLiteral("isMac"), false);
        engine->rootContext()->setContextProperty(QStringLiteral("isWindows"), false);
        engine->rootContext()->setContextProperty(QStringLiteral("isTails"), false);
        engine->rootContext()->setContextProperty(QStringLiteral("isOpenGL"), false);
        engine->rootContext()->setContextProperty(QStringLiteral("qtRuntimeVersion"), QString::fromLatin1(qVersion()));
        // Match the product identity displayed by main.cpp; keep wire identity separate.
        engine->rootContext()->setContextProperty(QStringLiteral("embeddedCoreVersion"), QWERTYCOIN_VERSION_FULL);
        engine->rootContext()->setContextProperty(QStringLiteral("embeddedCoreProductVersion"), QWERTYCOIN_VERSION);
        engine->rootContext()->setContextProperty(QStringLiteral("embeddedCoreCompatibleVersion"), QWERTYCOIN_COMPATIBLE_VERSION);
        engine->rootContext()->setContextProperty(QStringLiteral("legacyCoreVersion"), MONERO_VERSION_FULL);
    }

private:
    OSHelper m_osHelper;
    MessengerTestAccess m_messengerTests;
    QTemporaryDir m_accountsDir;
};

bool runQmlTestsIfRequested(int argc, char *argv[], int &result)
{
    bool requested = false;
    QVector<char *> testArgv;
    testArgv.reserve(argc);

    for (int i = 0; i < argc; ++i) {
        if (QString::fromLocal8Bit(argv[i]) == QStringLiteral("--test-qml")) {
            requested = true;
        } else {
            testArgv.append(argv[i]);
        }
    }

    if (!requested)
        return false;

    int testArgc = testArgv.size();
    QmlTestSetup setup;
    result = quick_test_main_with_setup(
        testArgc, testArgv.data(), "qwertycoin-gui-qml-tests", QML_TEST_SOURCE_DIR, &setup);
    return true;
}

#include "QmlTestHarness.moc"
