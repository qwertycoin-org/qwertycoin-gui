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
    Q_PROPERTY(int preparedTransactionCount READ preparedTransactionCount NOTIFY planChanged)
    Q_PROPERTY(quint64 preparedFee READ preparedFee NOTIFY planChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit Messenger(Monero::Wallet *wallet, QObject *parent = nullptr);
    ~Messenger() override;

    QString ownInvitation() const;
    QString ownFingerprint() const;
    QVariantList contacts() const;
    QVariantList messages() const;
    int preparedTransactionCount() const;
    quint64 preparedFee() const;
    QString status() const { return m_status; }

    Q_INVOKABLE bool importInvitation(const QString &label, const QString &encodedHex);
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

private:
    void ensureIdentity();
    void load();
    void save();
    void setStatus(const QString &value);
    qwertycoin::qms::hash32 genesis() const;
    qwertycoin::qms::invitation ownInvitationValue() const;
    static QByteArray bytes(const qwertycoin::qms::bytes &value);
    static qwertycoin::qms::bytes bytes(const QByteArray &value);
    static QString hex(const uint8_t *data, size_t size);
    static QByteArray unhex(const QString &value);

    Monero::Wallet *m_wallet;
    qwertycoin::qms::identity m_identity;
    qwertycoin::qms::invitation m_invitation;
    QJsonArray m_contacts;
    QJsonArray m_messages;
    QJsonObject m_incomplete;
    Monero::PendingTransaction *m_prepared = nullptr;
    QString m_preparedMessageId;
    QByteArray m_preparedJournal;
    QString m_status;
};
