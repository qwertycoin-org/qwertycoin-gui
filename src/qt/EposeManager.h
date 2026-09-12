// Copyright (c) 2026, The Qwertycoin Project
//
// All rights reserved.

#pragma once

#include <QObject>
#include <QFutureWatcher>
#include <QVariantList>
#include <QVariantMap>

class EposeManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString daemonEndpoint READ daemonEndpoint NOTIFY connectionChanged)
    Q_PROPERTY(bool localManaged READ localManaged NOTIFY connectionChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString state READ state NOTIFY snapshotChanged)
    Q_PROPERTY(QString lastSuccessfulAt READ lastSuccessfulAt NOTIFY snapshotChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantMap info READ info NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantMap epoch READ epoch NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantList serviceNodes READ serviceNodes NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantMap rewardPreview READ rewardPreview NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantMap legacyRegistration READ legacyRegistration NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantMap sourceStatus READ sourceStatus NOTIFY snapshotChanged)
    Q_PROPERTY(bool listTruncated READ listTruncated NOTIFY snapshotChanged)

public:
    explicit EposeManager(QObject *parent = nullptr);
    ~EposeManager() override;

    QString daemonEndpoint() const { return m_daemonEndpoint; }
    bool localManaged() const { return m_localManaged; }
    bool loading() const { return m_loading; }
    QString state() const { return m_state; }
    QString lastSuccessfulAt() const { return m_lastSuccessfulAt; }
    QString lastError() const { return m_lastError; }
    QVariantMap info() const { return m_info; }
    QVariantMap epoch() const { return m_epoch; }
    QVariantList serviceNodes() const { return m_serviceNodes; }
    QVariantMap rewardPreview() const { return m_rewardPreview; }
    QVariantMap legacyRegistration() const { return m_legacyRegistration; }
    QVariantMap sourceStatus() const { return m_sourceStatus; }
    bool listTruncated() const { return m_listTruncated; }

    // Credentials are accepted only for the request and are never exposed as a
    // QML property, written to settings, or logged by this adapter.
    Q_INVOKABLE void configure(const QString &endpoint,
                               const QString &username = QString(),
                               const QString &password = QString(),
                               bool localManaged = false);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void clear();

signals:
    void connectionChanged();
    void loadingChanged();
    void snapshotChanged();

private slots:
    void onRefreshFinished();

private:
    void startRefresh();
    void setLoading(bool loading);
    static QVariantMap fetchSnapshot(const QString &endpoint,
                                     const QString &username,
                                     const QString &password,
                                     quint64 generation);

    QString m_daemonEndpoint;
    QString m_username;
    QString m_password;
    bool m_localManaged = false;
    bool m_loading = false;
    bool m_refreshQueued = false;
    quint64 m_generation = 0;

    QString m_state = QStringLiteral("unknown");
    QString m_lastSuccessfulAt;
    QString m_lastError;
    QVariantMap m_info;
    QVariantMap m_epoch;
    QVariantList m_serviceNodes;
    QVariantMap m_rewardPreview;
    QVariantMap m_legacyRegistration;
    QVariantMap m_sourceStatus;
    bool m_listTruncated = false;

    QFutureWatcher<QVariantMap> m_watcher;
};
