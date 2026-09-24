#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>
#include <map>
#include <memory>

#include "wallet/api/wallet2_api.h"
#include "qms/protocol.h"
#include "qms/crypto_backend.h"

class Messenger : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ownInvitation READ ownInvitation NOTIFY stateChanged)
    Q_PROPERTY(QString ownFingerprint READ ownFingerprint NOTIFY stateChanged)
    Q_PROPERTY(QVariantList contacts READ contacts NOTIFY stateChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY stateChanged)
    Q_PROPERTY(int preparedTransactionCount READ preparedTransactionCount NOTIFY planChanged)
    Q_PROPERTY(quint64 preparedFee READ preparedFee NOTIFY planChanged)
    Q_PROPERTY(QString preparedContactFingerprint READ preparedContactFingerprint NOTIFY planChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool historyEnabled READ historyEnabled WRITE setHistoryEnabled NOTIFY historyEnabledChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)

public:
    explicit Messenger(Monero::Wallet *wallet, QObject *parent = nullptr);
    ~Messenger() override;

    QString ownInvitation() const;
    QString ownFingerprint() const;
    QVariantList contacts() const;
    QVariantList messages() const;
    int preparedTransactionCount() const;
    quint64 preparedFee() const;
    QString preparedContactFingerprint() const { return m_preparedContactFingerprint; }
    QString status() const { return m_status; }
    bool historyEnabled() const { return m_historyEnabled; }
    bool ready() const { return m_ready; }

    bool initialize();
    Q_INVOKABLE bool importInvitation(const QString &label, const QString &encodedHex);
    Q_INVOKABLE bool renameContact(const QString &fingerprint, const QString &label);
    Q_INVOKABLE bool removeContact(const QString &fingerprint);
    Q_INVOKABLE bool prepare(const QString &contactFingerprint, const QString &text);
    Q_INVOKABLE bool commitPrepared();
    Q_INVOKABLE void cancelPrepared();
    Q_INVOKABLE void ingestCarrier(quint64 height, const QString &blockHash,
                                   const QString &txId, const QString &extraHex);
    Q_INVOKABLE void handleReorg(quint64 height, quint64 blocksDetached);
    Q_INVOKABLE void setHistoryEnabled(bool enabled);
    Q_INVOKABLE void clearHistory();

signals:
    void stateChanged();
    void planChanged();
    void statusChanged();
    void historyEnabledChanged();
    void readyChanged();

private:
    void ensureIdentity();
    void load();
    bool save();
    void setStatus(const QString &value);
    std::string stateContext() const;
    qwertycoin::qms::hash32 genesis() const;
    static QByteArray bytes(const qwertycoin::qms::bytes &value);
    static qwertycoin::qms::bytes bytes(const QByteArray &value);
    static QString hex(const uint8_t *data, size_t size);
    static QByteArray unhex(const QString &value);

    Monero::Wallet *m_wallet;
    std::unique_ptr<qwertycoin::qms::crypto_backend> m_crypto;
    qwertycoin::qms::id16 m_invitationId{};
    qwertycoin::qms::hash32 m_fingerprint{};
    QByteArray m_contactPackage;
    QJsonArray m_contacts;
    QJsonArray m_messages;
    QJsonObject m_incomplete;
    QJsonArray m_seenMessages;
    bool m_historyEnabled = false;
    bool m_ready = false;
    Monero::PendingTransaction *m_prepared = nullptr;
    QString m_preparedMessageId;
    QString m_preparedContactFingerprint;
    QByteArray m_preparedJournal;
    QString m_status;
};
