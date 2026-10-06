#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>
#include <map>

#include "wallet/api/wallet2_api.h"
#include "qms/protocol.h"

class Messenger : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ownInvitation READ ownInvitation NOTIFY stateChanged)
    Q_PROPERTY(QString ownFingerprint READ ownFingerprint NOTIFY stateChanged)
    Q_PROPERTY(QVariantList contacts READ contacts NOTIFY stateChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY stateChanged)
    Q_PROPERTY(QVariantList transactionHistoryGroups READ transactionHistoryGroups NOTIFY stateChanged)
    Q_PROPERTY(int preparedTransactionCount READ preparedTransactionCount NOTIFY planChanged)
    Q_PROPERTY(quint64 preparedFee READ preparedFee NOTIFY planChanged)
    Q_PROPERTY(QString preparedContactFingerprint READ preparedContactFingerprint NOTIFY planChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool available READ available NOTIFY availabilityChanged)
    Q_PROPERTY(bool canCancelPrepared READ canCancelPrepared NOTIFY planChanged)

public:
    explicit Messenger(Monero::Wallet *wallet, QObject *parent = nullptr);
    ~Messenger() override;

    QString ownInvitation() const;
    QString ownFingerprint() const;
    QVariantList contacts() const;
    QVariantList messages() const;
    QVariantList transactionHistoryGroups() const;
    int preparedTransactionCount() const;
    quint64 preparedFee() const;
    QString preparedContactFingerprint() const { return m_preparedContactFingerprint; }
    QString status() const { return m_status; }
    bool available() const;
    bool canCancelPrepared() const { return m_prepared && !m_broadcastAttempted; }

    bool initialize();
    void refreshAvailability();

    Q_INVOKABLE bool importInvitation(const QString &label, const QString &encodedHex);
    Q_INVOKABLE bool confirmContact(const QString &fingerprint, const QString &confirmation);
    Q_INVOKABLE bool renameContact(const QString &fingerprint, const QString &label);
    Q_INVOKABLE bool removeContact(const QString &fingerprint);
    Q_INVOKABLE bool prepare(const QString &contactFingerprint, const QString &text);
    Q_INVOKABLE bool commitPrepared();
    Q_INVOKABLE void cancelPrepared();
    Q_INVOKABLE void ingestCarrier(quint64 height, const QString &blockHash,
                                   const QString &txId, const QString &extraHex);
    Q_INVOKABLE void handleReorg(quint64 height, quint64 blocksDetached);

signals:
    void stateChanged();
    void planChanged();
    void statusChanged();
    void availabilityChanged();

private:
    void ensureIdentity();
    void load();
    bool save();
    void clearSession();
    bool requireReady();
    bool openStoredCiphertext(const QString &id, const QJsonObject &candidate);
    void retryUnknown();
    bool messageExists(const QString &id, bool confirmedOnly = false) const;
    void setStatus(const QString &value);
    qwertycoin::qms::hash32 genesis() const;
    qwertycoin::qms::invitation ownInvitationValue() const;
    static QByteArray bytes(const qwertycoin::qms::bytes &value);
    static qwertycoin::qms::bytes bytes(const QByteArray &value);
    static QString hex(const uint8_t *data, size_t size);
    static QByteArray unhex(const QString &value);
    static QString normalizeFingerprint(const QString &value);

    Monero::Wallet *m_wallet;
    qwertycoin::qms::identity m_identity;
    qwertycoin::qms::invitation m_invitation;
    QJsonArray m_contacts;
    QJsonArray m_messages;
    QJsonObject m_incomplete;
    QJsonObject m_unknown;
    Monero::PendingTransaction *m_prepared = nullptr;
    QString m_preparedMessageId;
    QString m_preparedContactFingerprint;
    QByteArray m_preparedJournal;
    QString m_status;
    bool m_ready = false;
    bool m_broadcastAttempted = false;
    int m_preparedTotalTransactions = 0;
};
