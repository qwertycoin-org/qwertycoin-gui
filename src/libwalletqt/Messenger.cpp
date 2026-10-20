#include "Messenger.h"

#include <QDateTime>
#include <QHash>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QVariantMap>
#include <algorithm>
#include <cstring>
#include <sodium.h>

#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_core/cryptonote_tx_utils.h"

namespace
{
    const char *STATE_KEY = "qms/state/v1";
    const char *DOMAIN_STATUS_PREPARED = "prepared";
    constexpr int STATE_SCHEMA = 2;
    constexpr int MAX_CONTACTS = 1000;
    constexpr int MAX_MESSAGES = 10000;
    constexpr int MAX_INCOMPLETE_MESSAGES = 64;
    constexpr int MAX_UNKNOWN_MESSAGES = 32;
    constexpr int MAX_CHAIN_CANDIDATES = 4096;
    constexpr int MAX_CHAIN_HISTORY_CARRIERS = 10000;

    QJsonArray boundedArray(const QJsonArray &source, int limit)
    {
        QJsonArray result;
        for (int index = 0; index < source.size() && index < limit; ++index)
            result.push_back(source[index]);
        return result;
    }

    QJsonObject boundedObject(const QJsonObject &source, int limit)
    {
        QJsonObject result;
        for (auto it = source.begin(); it != source.end() && result.size() < limit; ++it)
            result.insert(it.key(), it.value());
        return result;
    }

    QString normalizedTransactionId(const QString &value)
    {
        const QByteArray candidate = value.trimmed().toLatin1().toLower();
        if (candidate.size() != 64)
            return {};
        const QByteArray decoded = QByteArray::fromHex(candidate);
        if (decoded.size() != 32 || decoded.toHex() != candidate)
            return {};
        return QString::fromLatin1(candidate);
    }

    QString normalizedMessageId(const QString &value)
    {
        const QByteArray candidate = value.trimmed().toLatin1().toLower();
        if (candidate.size() != 32)
            return {};
        const QByteArray decoded = QByteArray::fromHex(candidate);
        if (decoded.size() != 16 || decoded.toHex() != candidate)
            return {};
        return QString::fromLatin1(candidate);
    }
}

Messenger::Messenger(Monero::Wallet *wallet, QObject *parent)
    : QObject(parent), m_wallet(wallet)
{
    initialize();
}

Messenger::~Messenger()
{
    if (m_prepared)
        m_wallet->disposeTransaction(m_prepared);
    sodium_memzero(&m_identity, sizeof(m_identity));
}

bool Messenger::available() const
{
    return m_wallet->qmsStateStorageAvailable();
}

bool Messenger::initialize()
{
    if (m_ready)
        return available();
    if (!available()) {
        setStatus(tr("Messenger requires an unlocked full wallet with a non-empty session password."));
        emit availabilityChanged();
        return false;
    }

    try {
        load();
        ensureIdentity();
        m_ready = true;
    } catch (const std::exception &error) {
        sodium_memzero(&m_identity, sizeof(m_identity));
        m_invitation = {};
        setStatus(tr("Messenger initialization failed: ") + error.what());
        emit availabilityChanged();
        return false;
    }

    if (!m_preparedJournal.isEmpty()) {
        m_prepared = m_wallet->restoreQmsCarrierTransactions(m_preparedJournal.toStdString());
        if (!m_prepared || m_prepared->status() != Monero::PendingTransaction::Status_Ok) {
            if (m_prepared)
                m_wallet->disposeTransaction(m_prepared);
            m_prepared = nullptr;
            m_broadcastAttempted = true;
            setStatus(tr("A saved Messenger transaction plan could not be restored. Do not rebuild it until recovery has been reviewed."));
        }
    }

    retryUnknown();
    emit availabilityChanged();
    emit stateChanged();
    emit planChanged();
    return true;
}

void Messenger::refreshAvailability()
{
    if (!available()) {
        if (m_ready)
            clearSession();
        setStatus(tr("Messenger requires an unlocked full wallet with a non-empty session password."));
        emit availabilityChanged();
        return;
    }
    if (!m_ready)
        initialize();
    emit availabilityChanged();
}

void Messenger::clearSession()
{
    if (m_prepared)
        m_wallet->disposeTransaction(m_prepared);
    m_prepared = nullptr;
    sodium_memzero(&m_identity, sizeof(m_identity));
    m_invitation = {};
    m_contacts = {};
    m_messages = {};
    m_incomplete = {};
    m_unknown = {};
    m_chainCandidates = {};
    m_chainHistory = {};
    m_preparedJournal.clear();
    m_preparedMessageId.clear();
    m_preparedContactFingerprint.clear();
    m_preparedTotalTransactions = 0;
    m_broadcastAttempted = false;
    m_ready = false;
    emit stateChanged();
    emit planChanged();
}

bool Messenger::requireReady()
{
    if (!available()) {
        setStatus(tr("Messenger requires an unlocked full wallet with a non-empty session password."));
        return false;
    }
    return m_ready || initialize();
}

QByteArray Messenger::bytes(const qwertycoin::qms::bytes &value)
{
    return QByteArray(reinterpret_cast<const char *>(value.data()), int(value.size()));
}

qwertycoin::qms::bytes Messenger::bytes(const QByteArray &value)
{
    const auto *first = reinterpret_cast<const uint8_t *>(value.constData());
    return qwertycoin::qms::bytes(first, first + value.size());
}

QString Messenger::hex(const uint8_t *data, size_t size)
{
    return QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(data), int(size)).toHex());
}

QByteArray Messenger::unhex(const QString &value)
{
    const QByteArray latin = value.toLatin1();
    if ((latin.size() & 1) != 0)
        return {};
    const QByteArray result = QByteArray::fromHex(latin);
    return result.size() * 2 == latin.size() ? result : QByteArray{};
}

QString Messenger::normalizeFingerprint(const QString &value)
{
    QString result;
    result.reserve(value.size());
    for (const QChar character : value) {
        if (character.isSpace() || character == QLatin1Char(':') || character == QLatin1Char('-'))
            continue;
        result.append(character.toLower());
    }
    return result;
}

qwertycoin::qms::hash32 Messenger::genesis() const
{
    cryptonote::block block;
    const auto nettype = static_cast<cryptonote::network_type>(m_wallet->nettype());
    const auto &config = cryptonote::get_config(nettype);
    if (!cryptonote::generate_genesis_block(block, config.GENESIS_TX, config.GENESIS_NONCE))
        throw std::runtime_error("failed to derive network genesis");
    const crypto::hash hash = cryptonote::get_block_hash(block);
    qwertycoin::qms::hash32 result{};
    std::memcpy(result.data(), &hash, result.size());
    return result;
}

void Messenger::ensureIdentity()
{
    const bool valid = m_invitation.version == qwertycoin::qms::WIRE_VERSION &&
        qwertycoin::qms::verify_invitation(m_invitation) &&
        m_invitation.genesis == genesis() &&
        std::equal(m_invitation.box_public.begin(), m_invitation.box_public.end(), m_identity.box_public.begin()) &&
        std::equal(m_invitation.sign_public.begin(), m_invitation.sign_public.end(), m_identity.sign_public.begin());
    if (valid)
        return;

    sodium_memzero(&m_identity, sizeof(m_identity));
    m_identity = qwertycoin::qms::generate_identity();
    m_invitation = qwertycoin::qms::create_invitation(m_identity, genesis());
    if (!save())
        throw std::runtime_error("failed to persist encrypted Messenger identity");
}

void Messenger::load()
{
    const QByteArray raw = QByteArray::fromStdString(m_wallet->getCacheAttribute(STATE_KEY));
    const QJsonDocument document = QJsonDocument::fromJson(raw);
    if (!document.isObject())
        return;

    const QJsonObject root = document.object();
    auto copy = [](const QString &encoded, uint8_t *destination, size_t size) {
        const QByteArray decoded = QByteArray::fromBase64(encoded.toLatin1());
        if (size_t(decoded.size()) != size)
            return false;
        std::memcpy(destination, decoded.constData(), size);
        return true;
    };

    const QJsonObject identity = root.value("identity").toObject();
    bool valid = copy(identity.value("boxPublic").toString(), m_identity.box_public.data(), m_identity.box_public.size()) &&
        copy(identity.value("boxSecret").toString(), m_identity.box_secret.data(), m_identity.box_secret.size()) &&
        copy(identity.value("signPublic").toString(), m_identity.sign_public.data(), m_identity.sign_public.size()) &&
        copy(identity.value("signSecret").toString(), m_identity.sign_secret.data(), m_identity.sign_secret.size());
    if (valid) {
        try {
            m_invitation = qwertycoin::qms::decode_invitation(
                bytes(QByteArray::fromBase64(root.value("invitation").toString().toLatin1())));
            valid = m_invitation.genesis == genesis();
        } catch (...) {
            valid = false;
        }
    }
    if (!valid) {
        sodium_memzero(&m_identity, sizeof(m_identity));
        m_invitation = {};
    }

    m_contacts = boundedArray(root.value("contacts").toArray(), MAX_CONTACTS);
    if (root.value("schema").toInt() < STATE_SCHEMA) {
        // Earlier pre-release builds trusted imports automatically. Make users
        // explicitly re-confirm those fingerprints before another send.
        for (int index = 0; index < m_contacts.size(); ++index) {
            QJsonObject contact = m_contacts[index].toObject();
            contact["confirmed"] = false;
            m_contacts[index] = contact;
        }
    }
    m_messages = boundedArray(root.value("messages").toArray(), MAX_MESSAGES);
    m_incomplete = boundedObject(root.value("incomplete").toObject(), MAX_INCOMPLETE_MESSAGES);
    m_unknown = boundedObject(root.value("unknown").toObject(), MAX_UNKNOWN_MESSAGES);
    m_chainHistory = boundedObject(root.value("chainHistory").toObject(), MAX_CHAIN_HISTORY_CARRIERS);
    m_preparedJournal = QByteArray::fromBase64(root.value("preparedJournal").toString().toLatin1());
    m_preparedMessageId = root.value("preparedMessageId").toString();
    m_preparedContactFingerprint = root.value("preparedContactFingerprint").toString();
    m_broadcastAttempted = root.value("broadcastAttempted").toBool(false);
    m_preparedTotalTransactions = root.value("preparedTotalTransactions").toInt(0);
    if (!m_preparedMessageId.isEmpty() && m_preparedContactFingerprint.isEmpty()) {
        for (const auto value : m_messages) {
            const QJsonObject message = value.toObject();
            if (message.value("id").toString() == m_preparedMessageId) {
                m_preparedContactFingerprint = message.value("contact").toString();
                break;
            }
        }
    }
}

bool Messenger::save()
{
    if (!available() || !qwertycoin::qms::verify_invitation(m_invitation))
        return false;

    auto b64 = [](const uint8_t *data, size_t size) {
        return QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(data), int(size)).toBase64());
    };
    QJsonObject identity;
    identity["boxPublic"] = b64(m_identity.box_public.data(), m_identity.box_public.size());
    identity["boxSecret"] = b64(m_identity.box_secret.data(), m_identity.box_secret.size());
    identity["signPublic"] = b64(m_identity.sign_public.data(), m_identity.sign_public.size());
    identity["signSecret"] = b64(m_identity.sign_secret.data(), m_identity.sign_secret.size());

    QJsonObject root;
    root["schema"] = STATE_SCHEMA;
    root["identity"] = identity;
    root["invitation"] = QString::fromLatin1(bytes(qwertycoin::qms::encode_invitation(m_invitation)).toBase64());
    root["contacts"] = m_contacts;
    root["messages"] = m_messages;
    root["incomplete"] = m_incomplete;
    root["unknown"] = m_unknown;
    root["chainHistory"] = m_chainHistory;
    root["preparedJournal"] = QString::fromLatin1(m_preparedJournal.toBase64());
    root["preparedMessageId"] = m_preparedMessageId;
    root["preparedContactFingerprint"] = m_preparedContactFingerprint;
    root["broadcastAttempted"] = m_broadcastAttempted;
    root["preparedTotalTransactions"] = m_preparedTotalTransactions;
    if (!m_wallet->setCacheAttribute(STATE_KEY, QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString()))
        return false;
    // setCacheAttribute updates wallet2's in-memory attribute map only. QMS
    // identities and, critically, signed transaction recovery journals must
    // reach the encrypted wallet cache before the caller may continue.
    if (!m_wallet->store(""))
        return false;
    emit stateChanged();
    return true;
}

qwertycoin::qms::invitation Messenger::ownInvitationValue() const
{
    return m_invitation;
}

QString Messenger::ownInvitation() const
{
    if (!m_ready || !available())
        return {};
    return QString::fromStdString(qwertycoin::qms::hex(qwertycoin::qms::encode_invitation(m_invitation)));
}

QString Messenger::ownFingerprint() const
{
    if (!m_ready || !available())
        return {};
    const auto value = qwertycoin::qms::fingerprint(m_identity.box_public, m_identity.sign_public);
    return hex(value.data(), value.size());
}

QVariantList Messenger::contacts() const
{
    QVariantList result;
    if (!m_ready || !available())
        return result;
    for (const auto value : m_contacts)
        result.push_back(value.toObject().toVariantMap());
    return result;
}

QVariantList Messenger::messages() const
{
    QVariantList result;
    if (!m_ready || !available())
        return result;
    for (const auto value : m_messages)
        result.push_back(value.toObject().toVariantMap());
    return result;
}

QVariantList Messenger::transactionHistoryGroups() const
{
    QVariantList result;
    if (!m_ready || !available())
        return result;

    struct Candidate {
        QJsonObject message;
        QStringList transactionIds;
    };
    QList<Candidate> candidates;
    QHash<QString, int> claims;

    for (const auto value : m_messages) {
        const QJsonObject message = value.toObject();
        if (message.value("direction").toString() != QLatin1String("out"))
            continue;

        const QJsonArray storedIds = message.value("transactionIds").toArray();
        if (storedIds.isEmpty())
            continue;

        QStringList transactionIds;
        QSet<QString> uniqueIds;
        bool valid = true;
        for (const auto storedId : storedIds) {
            const QString transactionId = normalizedTransactionId(storedId.toString());
            if (transactionId.isEmpty()) {
                valid = false;
                continue;
            }
            ++claims[transactionId];
            if (uniqueIds.contains(transactionId)) {
                valid = false;
                continue;
            }
            uniqueIds.insert(transactionId);
            transactionIds.push_back(transactionId);
        }
        if (!valid)
            continue;
        candidates.push_back({message, transactionIds});
    }

    for (const Candidate &candidate : candidates) {
        bool unambiguous = true;
        for (const QString &transactionId : candidate.transactionIds) {
            if (claims.value(transactionId) != 1) {
                unambiguous = false;
                break;
            }
        }
        if (!unambiguous)
            continue;

        bool feeOk = false;
        const quint64 feeAtomic = candidate.message.value("fee").toString().toULongLong(&feeOk);
        QVariantMap group;
        group.insert("messageId", candidate.message.value("id").toString());
        group.insert("transactionIds", candidate.transactionIds);
        group.insert("transactionCount", candidate.transactionIds.size());
        group.insert("fee", feeOk ? QString::fromStdString(Monero::Wallet::displayAmount(feeAtomic)) : QString());
        group.insert("status", candidate.message.value("status").toString());
        group.insert("timestamp", candidate.message.value("timestamp").toString());
        result.push_back(group);
    }

    struct RecoveredGroup {
        int fragmentCount = 0;
        QMap<int, QString> transactionIds;
        bool valid = true;
    };
    QHash<QString, RecoveredGroup> recovered;
    for (auto it = m_chainHistory.begin(); it != m_chainHistory.end(); ++it) {
        const QString transactionId = normalizedTransactionId(it.key());
        const QJsonObject record = it.value().toObject();
        const QString storedTransactionId = normalizedTransactionId(record.value("transactionId").toString());
        const QString messageId = normalizedMessageId(record.value("messageId").toString());
        const QString blockHash = normalizedTransactionId(record.value("blockHash").toString());
        const int fragmentIndex = record.value("fragmentIndex").toInt(-1);
        const int fragmentCount = record.value("fragmentCount").toInt(0);
        bool heightOk = false;
        const quint64 height = record.value("height").toString().toULongLong(&heightOk);
        if (transactionId.isEmpty() || storedTransactionId != transactionId || messageId.isEmpty() ||
            blockHash.isEmpty() || !heightOk || height == 0 || fragmentCount < 1 ||
            fragmentCount > int(qwertycoin::qms::MAX_FRAGMENTS) || fragmentIndex < 0 ||
            fragmentIndex >= fragmentCount)
            continue;

        RecoveredGroup &group = recovered[messageId];
        if (group.fragmentCount != 0 && group.fragmentCount != fragmentCount)
            group.valid = false;
        group.fragmentCount = fragmentCount;
        if (group.transactionIds.contains(fragmentIndex) &&
            group.transactionIds.value(fragmentIndex) != transactionId)
            group.valid = false;
        group.transactionIds.insert(fragmentIndex, transactionId);
    }

    QHash<QString, int> resultIndexByMessageId;
    for (int index = 0; index < result.size(); ++index) {
        const QString messageId = result[index].toMap().value("messageId").toString();
        if (!messageId.isEmpty())
            resultIndexByMessageId.insert(messageId, index);
    }
    for (auto it = recovered.begin(); it != recovered.end(); ++it) {
        if (!it.value().valid || it.value().fragmentCount < 1 ||
            it.value().transactionIds.size() != it.value().fragmentCount)
            continue;
        const QStringList recoveredIds = it.value().transactionIds.values();
        const int existingIndex = resultIndexByMessageId.value(it.key(), -1);
        if (existingIndex >= 0) {
            QVariantMap group = result[existingIndex].toMap();
            QStringList transactionIds = group.value("transactionIds").toStringList();
            QSet<QString> seen;
            for (const QString &transactionId : transactionIds)
                seen.insert(transactionId);
            for (const QString &transactionId : recoveredIds) {
                if (!seen.contains(transactionId)) {
                    seen.insert(transactionId);
                    transactionIds.push_back(transactionId);
                }
            }
            group.insert("transactionIds", transactionIds);
            group.insert("transactionCount", transactionIds.size());
            result[existingIndex] = group;
            continue;
        }

        QVariantMap group;
        group.insert("messageId", it.key());
        group.insert("transactionIds", recoveredIds);
        group.insert("transactionCount", recoveredIds.size());
        group.insert("fee", QString());
        group.insert("status", QStringLiteral("confirmed"));
        group.insert("timestamp", QString());
        resultIndexByMessageId.insert(it.key(), result.size());
        result.push_back(group);
    }
    return result;
}

int Messenger::preparedTransactionCount() const
{
    return available() && m_prepared ? int(m_prepared->txCount()) : 0;
}

quint64 Messenger::preparedFee() const
{
    return available() && m_prepared ? m_prepared->fee() : 0;
}

void Messenger::setStatus(const QString &value)
{
    if (m_status == value)
        return;
    m_status = value;
    emit statusChanged();
}

bool Messenger::importInvitation(const QString &label, const QString &encodedHex)
{
    if (!requireReady())
        return false;
    try {
        if (m_contacts.size() >= MAX_CONTACTS)
            throw std::runtime_error("contact limit reached");
        const QString normalizedLabel = label.trimmed();
        if (normalizedLabel.isEmpty())
            throw std::runtime_error("contact name is required");
        const QByteArray raw = unhex(encodedHex.trimmed());
        if (raw.isEmpty())
            throw std::runtime_error("invitation is not canonical hexadecimal");
        const auto invitation = qwertycoin::qms::decode_invitation(bytes(raw));
        if (invitation.genesis != genesis())
            throw std::runtime_error("invitation belongs to another network");
        const auto fp = qwertycoin::qms::fingerprint(invitation.box_public, invitation.sign_public);
        const QString fingerprint = hex(fp.data(), fp.size());
        if (fingerprint == ownFingerprint())
            throw std::runtime_error("cannot import this wallet's own messenger invitation");
        for (const auto value : m_contacts) {
            if (value.toObject().value("fingerprint").toString() == fingerprint)
                throw std::runtime_error("contact fingerprint is already present");
        }

        QJsonObject contact;
        contact["label"] = normalizedLabel;
        contact["fingerprint"] = fingerprint;
        contact["invitation"] = encodedHex.trimmed().toLower();
        contact["confirmed"] = false;
        m_contacts.push_back(contact);
        if (!save()) {
            m_contacts.removeLast();
            throw std::runtime_error("encrypted contact state could not be persisted");
        }
        setStatus(tr("Contact imported but not verified. Compare the complete fingerprint out of band before sending."));
        return true;
    } catch (const std::exception &error) {
        setStatus(tr("Invitation rejected: ") + error.what());
        return false;
    }
}

bool Messenger::confirmContact(const QString &fingerprint, const QString &confirmation)
{
    if (!requireReady())
        return false;
    const QString expected = normalizeFingerprint(fingerprint);
    const QString supplied = normalizeFingerprint(confirmation);
    if (expected.size() != 64 || unhex(expected).size() != 32 || supplied != expected) {
        setStatus(tr("Fingerprint confirmation does not match the complete contact fingerprint."));
        return false;
    }
    for (int index = 0; index < m_contacts.size(); ++index) {
        QJsonObject contact = m_contacts[index].toObject();
        if (contact.value("fingerprint").toString() != expected)
            continue;
        const bool previous = contact.value("confirmed").toBool(false);
        contact["confirmed"] = true;
        m_contacts[index] = contact;
        if (!save()) {
            contact["confirmed"] = previous;
            m_contacts[index] = contact;
            setStatus(tr("Verified contact state could not be persisted."));
            return false;
        }
        retryUnknown();
        setStatus(tr("Contact fingerprint verified. Encrypted sending is now enabled."));
        return true;
    }
    setStatus(tr("Contact not found."));
    return false;
}

bool Messenger::renameContact(const QString &fingerprint, const QString &label)
{
    if (!requireReady())
        return false;
    const QString normalized = label.trimmed();
    if (normalized.isEmpty()) {
        setStatus(tr("Contact name cannot be empty."));
        return false;
    }
    const QJsonArray contactsBefore = m_contacts;
    const QJsonArray messagesBefore = m_messages;
    bool found = false;
    for (int index = 0; index < m_contacts.size(); ++index) {
        QJsonObject contact = m_contacts[index].toObject();
        if (contact.value("fingerprint").toString() != fingerprint)
            continue;
        contact["label"] = normalized;
        m_contacts[index] = contact;
        found = true;
        break;
    }
    if (!found) {
        setStatus(tr("Contact not found."));
        return false;
    }
    for (int index = 0; index < m_messages.size(); ++index) {
        QJsonObject message = m_messages[index].toObject();
        if (message.value("contact").toString() != fingerprint)
            continue;
        message["label"] = normalized;
        m_messages[index] = message;
    }
    if (!save()) {
        m_contacts = contactsBefore;
        m_messages = messagesBefore;
        setStatus(tr("Renamed contact state could not be persisted."));
        return false;
    }
    setStatus(tr("Contact renamed."));
    return true;
}

bool Messenger::removeContact(const QString &fingerprint)
{
    if (!requireReady())
        return false;
    if (!m_preparedContactFingerprint.isEmpty() && m_preparedContactFingerprint == fingerprint) {
        setStatus(tr("Resolve the prepared message before removing this contact."));
        return false;
    }
    for (int index = 0; index < m_contacts.size(); ++index) {
        if (m_contacts[index].toObject().value("fingerprint").toString() != fingerprint)
            continue;
        const QJsonValue removed = m_contacts[index];
        m_contacts.removeAt(index);
        if (!save()) {
            m_contacts.insert(index, removed);
            setStatus(tr("Contact removal could not be persisted."));
            return false;
        }
        setStatus(tr("Contact removed. Existing local message history was retained."));
        return true;
    }
    setStatus(tr("Contact not found."));
    return false;
}

bool Messenger::prepare(const QString &contactFingerprint, const QString &text)
{
    if (!requireReady())
        return false;
    if (m_prepared || !m_preparedJournal.isEmpty()) {
        setStatus(tr("Resolve the existing Messenger transaction plan before preparing another message."));
        return false;
    }

    try {
        qwertycoin::qms::invitation recipient;
        QString label;
        bool found = false;
        for (const auto value : m_contacts) {
            const QJsonObject contact = value.toObject();
            if (contact.value("fingerprint").toString() != contactFingerprint)
                continue;
            if (!contact.value("confirmed").toBool(false))
                throw std::runtime_error("contact fingerprint has not been verified");
            recipient = qwertycoin::qms::decode_invitation(bytes(unhex(contact.value("invitation").toString())));
            label = contact.value("label").toString();
            found = true;
            break;
        }
        if (!found)
            throw std::runtime_error("unknown messenger contact");

        const QByteArray utf8Bytes = text.toUtf8();
        if (utf8Bytes.isEmpty())
            throw std::runtime_error("message text is empty");
        if (size_t(utf8Bytes.size()) > qwertycoin::qms::MAX_TEXT_BYTES)
            throw std::runtime_error("message exceeds 4,096 UTF-8 bytes");

        qwertycoin::qms::id16 id{};
        randombytes_buf(id.data(), id.size());
        const auto ciphertext = qwertycoin::qms::seal_text(
            m_identity, recipient, genesis(), id, utf8Bytes.toStdString());
        const auto fragments = qwertycoin::qms::fragment_ciphertext(recipient, genesis(), id, ciphertext);
        std::vector<std::vector<uint8_t>> extras;
        for (const auto &fragment : fragments) {
            std::vector<uint8_t> extra;
            if (!qwertycoin::qms::append_carrier_nonces(extra, fragment))
                throw std::runtime_error("fragment does not fit unchanged tx_extra limit");
            extras.push_back(std::move(extra));
        }

        m_prepared = m_wallet->createQmsCarrierTransactions(extras, 1, m_wallet->defaultMixin());
        if (!m_prepared || m_prepared->status() != Monero::PendingTransaction::Status_Ok)
            throw std::runtime_error(m_prepared ? m_prepared->errorString() : "wallet returned no transaction plan");

        m_preparedMessageId = hex(id.data(), id.size());
        m_preparedContactFingerprint = contactFingerprint;
        m_preparedJournal = QByteArray::fromStdString(m_prepared->qmsJournalData());
        m_preparedTotalTransactions = int(m_prepared->txCount());
        m_broadcastAttempted = false;
        if (m_preparedJournal.isEmpty())
            throw std::runtime_error("wallet could not create encrypted QMS journal");

        QJsonArray transactionIds;
        QSet<QString> uniqueTransactionIds;
        for (const std::string &rawTransactionId : m_prepared->txid()) {
            const QString transactionId = normalizedTransactionId(QString::fromStdString(rawTransactionId));
            if (transactionId.isEmpty() || uniqueTransactionIds.contains(transactionId))
                throw std::runtime_error("wallet returned invalid or duplicate QMS transaction IDs");
            uniqueTransactionIds.insert(transactionId);
            transactionIds.push_back(transactionId);
        }
        if (transactionIds.size() != m_preparedTotalTransactions)
            throw std::runtime_error("wallet returned incomplete QMS transaction IDs");

        QJsonObject message;
        message["id"] = m_preparedMessageId;
        message["contact"] = contactFingerprint;
        message["label"] = label;
        message["text"] = text;
        message["direction"] = "out";
        message["status"] = DOMAIN_STATUS_PREPARED;
        message["transactions"] = m_preparedTotalTransactions;
        message["transactionIds"] = transactionIds;
        message["fee"] = QString::number(m_prepared->fee());
        message["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        m_messages.push_back(message);
        if (!save())
            throw std::runtime_error("encrypted transaction journal could not be persisted");

        setStatus(tr("Encrypted and prepared. Review transaction count and total fee, then send explicitly."));
        emit planChanged();
        return true;
    } catch (const std::exception &error) {
        if (m_prepared)
            m_wallet->disposeTransaction(m_prepared);
        for (int index = m_messages.size() - 1; index >= 0; --index) {
            if (m_messages[index].toObject().value("id").toString() == m_preparedMessageId)
                m_messages.removeAt(index);
        }
        m_prepared = nullptr;
        m_preparedMessageId.clear();
        m_preparedContactFingerprint.clear();
        m_preparedJournal.clear();
        m_preparedTotalTransactions = 0;
        m_broadcastAttempted = false;
        save();
        emit planChanged();
        setStatus(tr("Preparation failed: ") + error.what());
        return false;
    }
}

bool Messenger::commitPrepared()
{
    if (!requireReady())
        return false;
    if (!m_prepared) {
        setStatus(tr("No restorable prepared message is available."));
        return false;
    }

    m_broadcastAttempted = true;
    for (int index = 0; index < m_messages.size(); ++index) {
        QJsonObject message = m_messages[index].toObject();
        if (message.value("id").toString() == m_preparedMessageId) {
            message["status"] = "broadcasting";
            m_messages[index] = message;
            break;
        }
    }
    if (!save()) {
        setStatus(tr("Broadcast blocked because the recovery journal could not be persisted."));
        return false;
    }

    const bool ok = m_prepared->commit();
    const QString error = QString::fromStdString(m_prepared->errorString());
    const QByteArray remainingJournal = QByteArray::fromStdString(m_prepared->qmsJournalData());
    const int remainingTransactions = int(m_prepared->txCount());

    for (int index = 0; index < m_messages.size(); ++index) {
        QJsonObject message = m_messages[index].toObject();
        if (message.value("id").toString() != m_preparedMessageId)
            continue;
        message["status"] = ok ? "sent" : "broadcast outcome unknown";
        message["remainingTransactions"] = remainingTransactions;
        m_messages[index] = message;
        break;
    }

    m_wallet->disposeTransaction(m_prepared);
    m_prepared = nullptr;

    if (ok) {
        m_preparedMessageId.clear();
        m_preparedContactFingerprint.clear();
        m_preparedJournal.clear();
        m_preparedTotalTransactions = 0;
        m_broadcastAttempted = false;
        save();
        emit planChanged();
        setStatus(tr("Encrypted message transactions submitted."));
        return true;
    }

    m_preparedJournal = remainingJournal;
    if (!m_preparedJournal.isEmpty()) {
        m_prepared = m_wallet->restoreQmsCarrierTransactions(m_preparedJournal.toStdString());
        if (!m_prepared || m_prepared->status() != Monero::PendingTransaction::Status_Ok) {
            if (m_prepared)
                m_wallet->disposeTransaction(m_prepared);
            m_prepared = nullptr;
        }
    }
    save();
    emit planChanged();
    if (m_prepared) {
        setStatus(tr("Broadcast result is uncertain. Retry only this same signed plan; cancellation is disabled to prevent rebuilding and double fees. ") + error);
    } else {
        setStatus(tr("Broadcast result is uncertain and the remaining signed plan could not be restored. Do not rebuild the message until recovery is reviewed. ") + error);
    }
    return false;
}

void Messenger::cancelPrepared()
{
    if (!requireReady())
        return;
    if (m_broadcastAttempted) {
        setStatus(tr("Cancellation is disabled after a broadcast attempt because its outcome may be unknown."));
        return;
    }
    if (m_prepared)
        m_wallet->disposeTransaction(m_prepared);
    if (!m_preparedMessageId.isEmpty()) {
        for (int index = m_messages.size() - 1; index >= 0; --index) {
            const QJsonObject message = m_messages[index].toObject();
            if (message.value("id").toString() == m_preparedMessageId &&
                message.value("status").toString() == DOMAIN_STATUS_PREPARED)
                m_messages.removeAt(index);
        }
    }
    m_prepared = nullptr;
    m_preparedMessageId.clear();
    m_preparedContactFingerprint.clear();
    m_preparedJournal.clear();
    m_preparedTotalTransactions = 0;
    m_broadcastAttempted = false;
    save();
    emit planChanged();
    setStatus(tr("Prepared message cancelled without broadcasting."));
}

bool Messenger::messageExists(const QString &id, bool confirmedOnly) const
{
    for (const auto value : m_messages) {
        const QJsonObject message = value.toObject();
        if (message.value("id").toString() != id)
            continue;
        if (!confirmedOnly || message.value("status").toString() == QLatin1String("confirmed"))
            return true;
    }
    return false;
}

bool Messenger::openStoredCiphertext(const QString &id, const QJsonObject &candidate)
{
    const QByteArray rawCiphertext = QByteArray::fromBase64(candidate.value("ciphertext").toString().toLatin1());
    const QByteArray rawMessageId = unhex(id);
    if (rawCiphertext.isEmpty() || rawMessageId.size() != int(qwertycoin::qms::id16{}.size()))
        return false;

    qwertycoin::qms::id16 messageId{};
    std::memcpy(messageId.data(), rawMessageId.constData(), messageId.size());
    for (const auto value : m_contacts) {
        const QJsonObject contact = value.toObject();
        if (!contact.value("confirmed").toBool(false))
            continue;
        try {
            const auto sender = qwertycoin::qms::decode_invitation(
                bytes(unhex(contact.value("invitation").toString())));
            const auto opened = qwertycoin::qms::open_text(
                m_identity, sender, m_invitation, genesis(), messageId, bytes(rawCiphertext));

            QJsonObject message;
            message["id"] = id;
            message["contact"] = contact.value("fingerprint");
            message["label"] = contact.value("label");
            message["text"] = QString::fromUtf8(opened.text.data(), int(opened.text.size()));
            message["direction"] = "in";
            message["status"] = "confirmed";
            message["height"] = candidate.value("height");
            message["blockHash"] = candidate.value("blockHash");
            message["txIds"] = candidate.value("txIds");
            message["timestamp"] = candidate.value("timestamp");

            for (int index = 0; index < m_messages.size(); ++index) {
                const QJsonObject existing = m_messages[index].toObject();
                if (existing.value("id").toString() != id)
                    continue;
                if (existing.value("direction").toString() == QLatin1String("in") &&
                    existing.value("status").toString() == QLatin1String("chain proof lost"))
                    m_messages[index] = message;
                return true;
            }
            if (m_messages.size() >= MAX_MESSAGES) {
                setStatus(tr("Messenger history limit reached; a verified message remains queued."));
                return false;
            }
            m_messages.push_back(message);
            return true;
        } catch (...) {
            // Try the next explicitly verified and pinned contact.
        }
    }
    return false;
}

void Messenger::retryUnknown()
{
    if (m_unknown.isEmpty())
        return;
    const QStringList ids = m_unknown.keys();
    bool changed = false;
    for (const QString &id : ids) {
        if (!openStoredCiphertext(id, m_unknown.value(id).toObject()))
            continue;
        m_unknown.remove(id);
        changed = true;
    }
    if (changed)
        save();
}

void Messenger::ingestCarrier(quint64 height, const QString &blockHash,
                              const QString &txId, const QString &extraHex)
{
    if (!requireReady())
        return;
    try {
        const auto fragments = qwertycoin::qms::extract_carrier_fragments(bytes(unhex(extraHex)));
        if (fragments.size() != 1)
            return;

        const auto &fragment = fragments[0];
        const QString transactionId = normalizedTransactionId(txId);
        const QString normalizedBlockHash = normalizedTransactionId(blockHash);
        const QString id = hex(fragment.message_id.data(), fragment.message_id.size());
        if (height == 0 || transactionId.isEmpty() || normalizedBlockHash.isEmpty() ||
            normalizedMessageId(id).isEmpty())
            return;

        QJsonObject chainCandidate;
        chainCandidate["transactionId"] = transactionId;
        chainCandidate["messageId"] = id;
        chainCandidate["fragmentIndex"] = int(fragment.index);
        chainCandidate["fragmentCount"] = int(fragment.count);
        chainCandidate["height"] = QString::number(height);
        chainCandidate["blockHash"] = normalizedBlockHash;
        if (!m_chainHistory.contains(transactionId)) {
            const QJsonObject existing = m_chainCandidates.value(transactionId).toObject();
            if (!existing.isEmpty() && existing != chainCandidate) {
                m_chainCandidates.remove(transactionId);
                return;
            }
            if (existing.isEmpty() && m_chainCandidates.size() < MAX_CHAIN_CANDIDATES)
                m_chainCandidates.insert(transactionId, chainCandidate);
        }

        if (!qwertycoin::qms::verify_fragment(m_invitation, genesis(), fragment))
            return;

        if (messageExists(id, true))
            return;

        QJsonArray parts = m_incomplete.value(id).toArray();
        const QString encoded = QString::fromLatin1(bytes(qwertycoin::qms::encode_fragment(fragment)).toBase64());
        for (const auto value : parts) {
            const QJsonObject existing = value.toObject();
            if (existing.value("index").toInt() != fragment.index)
                continue;
            if (existing.value("data").toString() != encoded) {
                m_incomplete.remove(id);
                save();
            }
            return;
        }
        if (m_incomplete.size() >= MAX_INCOMPLETE_MESSAGES && !m_incomplete.contains(id)) {
            setStatus(tr("Messenger incomplete-message limit reached; new unauthenticated fragments are ignored."));
            return;
        }

        QJsonObject part;
        part["index"] = fragment.index;
        part["data"] = encoded;
        part["height"] = QString::number(height);
        part["blockHash"] = blockHash;
        part["txId"] = txId;
        parts.push_back(part);
        m_incomplete[id] = parts;
        if (parts.size() != fragment.count) {
            if (!save())
                setStatus(tr("Messenger fragment state could not be persisted."));
            return;
        }

        std::vector<qwertycoin::qms::fragment> complete;
        QJsonArray txIds;
        for (const auto value : parts) {
            const QJsonObject stored = value.toObject();
            complete.push_back(qwertycoin::qms::decode_fragment(
                bytes(QByteArray::fromBase64(stored.value("data").toString().toLatin1()))));
            txIds.push_back(stored.value("txId"));
        }
        const auto ciphertext = qwertycoin::qms::reassemble(complete);
        QJsonObject candidate;
        candidate["ciphertext"] = QString::fromLatin1(bytes(ciphertext).toBase64());
        candidate["height"] = QString::number(height);
        candidate["blockHash"] = blockHash;
        candidate["txIds"] = txIds;
        candidate["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        m_incomplete.remove(id);
        if (!openStoredCiphertext(id, candidate)) {
            if (!m_unknown.contains(id) && m_unknown.size() >= MAX_UNKNOWN_MESSAGES) {
                setStatus(tr("Messenger unknown-sender queue is full; import and verify contacts before rescanning."));
            } else {
                m_unknown[id] = candidate;
            }
        } else {
            m_unknown.remove(id);
        }
        if (!save())
            setStatus(tr("Confirmed Messenger state could not be persisted."));
    } catch (...) {
        // Malformed or unauthenticated foreign chain data is deliberately ignored.
    }
}

void Messenger::reconcileTransactionHistory()
{
    if (!m_ready || !available() || m_chainCandidates.isEmpty())
        return;

    try {
        Monero::TransactionHistory *history = m_wallet->history();
        if (!history)
            return;
        history->refresh();

        QSet<QString> outgoingSelfTransfers;
        for (Monero::TransactionInfo *transaction : history->getAll()) {
            if (!transaction || transaction->direction() != Monero::TransactionInfo::Direction_Out ||
                transaction->isPending() || transaction->isFailed() || transaction->amount() != 0)
                continue;
            const QString transactionId = normalizedTransactionId(QString::fromStdString(transaction->hash()));
            if (!transactionId.isEmpty())
                outgoingSelfTransfers.insert(transactionId);
        }

        const QJsonObject previousChainHistory = m_chainHistory;
        bool changed = false;
        for (const QString &transactionId : outgoingSelfTransfers) {
            if (m_chainHistory.contains(transactionId))
                continue;
            const QJsonObject candidate = m_chainCandidates.value(transactionId).toObject();
            if (candidate.isEmpty())
                continue;
            if (m_chainHistory.size() >= MAX_CHAIN_HISTORY_CARRIERS) {
                setStatus(tr("Messenger chain-history limit reached; additional carriers remain ordinary payments."));
                break;
            }
            m_chainHistory.insert(transactionId, candidate);
            changed = true;
        }

        if (changed && !save()) {
            m_chainHistory = previousChainHistory;
            setStatus(tr("Recovered Messenger history could not be persisted."));
            return;
        }
        m_chainCandidates = {};
    } catch (...) {
        // Keep the bounded in-memory candidates for the next completed refresh.
    }
}

void Messenger::handleReorg(quint64 height, quint64)
{
    if (!requireReady())
        return;
    bool changed = false;
    for (int index = 0; index < m_messages.size(); ++index) {
        QJsonObject message = m_messages[index].toObject();
        if (message.value("direction").toString() == QLatin1String("in") &&
            message.value("height").toString().toULongLong() >= height) {
            message["status"] = "chain proof lost";
            m_messages[index] = message;
            changed = true;
        }
    }
    for (const QString &id : m_unknown.keys()) {
        if (m_unknown.value(id).toObject().value("height").toString().toULongLong() >= height) {
            m_unknown.remove(id);
            changed = true;
        }
    }
    for (const QString &id : m_incomplete.keys()) {
        const QJsonArray parts = m_incomplete.value(id).toArray();
        bool detached = false;
        for (const auto value : parts) {
            if (value.toObject().value("height").toString().toULongLong() >= height) {
                detached = true;
                break;
            }
        }
        if (detached) {
            m_incomplete.remove(id);
            changed = true;
        }
    }
    auto removeDetachedCarriers = [height, &changed](QJsonObject &records) {
        for (auto it = records.begin(); it != records.end();) {
            bool heightOk = false;
            const quint64 recordHeight = it.value().toObject().value("height").toString().toULongLong(&heightOk);
            if (!heightOk || recordHeight >= height) {
                it = records.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }
    };
    removeDetachedCarriers(m_chainCandidates);
    removeDetachedCarriers(m_chainHistory);
    if (changed && !save())
        setStatus(tr("Messenger reorg state could not be persisted."));
}
