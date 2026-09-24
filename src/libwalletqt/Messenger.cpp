#include "Messenger.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QDateTime>
#include <QVariantMap>
#include <algorithm>
#include <cstring>
#include <stdexcept>

#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "string_tools.h"
#include "qms/wallet_state.h"

namespace
{
    const char *DOMAIN_STATUS_PREPARED = "prepared";
    const int MAX_INCOMPLETE_MESSAGES = 64;
    const int MAX_INCOMPLETE_PER_CONTACT = 16;
    const qint64 MAX_REASSEMBLY_BYTES = 8 * 1024 * 1024;
    const int MAX_SEEN_MESSAGES = 4096;
}

Messenger::Messenger(Monero::Wallet *wallet, QObject *parent)
    : QObject(parent), m_wallet(wallet)
{
    initialize();
}

bool Messenger::initialize()
{
    if (m_ready) return true;
    emit enabledChanged(); // Also refreshes canEnable after a password change.
    if (!m_wallet->qmsStateExists()) {
        setStatus(tr("Messenger is not enabled for this wallet."));
        return false;
    }
    return enable();
}

bool Messenger::enable()
{
    if (m_ready) return true;
    if (!m_wallet->qmsStateStorageAvailable()) {
        setStatus(tr("Messenger requires a password-protected, unlocked wallet."));
        return false;
    }
    if (!m_wallet->qmsStateExists() && !m_wallet->qmsStrictTransportReady()) {
        setStatus(tr("Configure a SOCKS proxy and Tor v3 onion daemon before enabling Messenger."));
        return false;
    }
    try {
        load();
        ensureIdentity();
        m_ready = true;
        emit enabledChanged();
        emit readyChanged();
        if (!m_preparedJournal.isEmpty()) {
            m_prepared = m_wallet->restoreQmsCarrierTransactions(m_preparedJournal.toStdString());
            if (!m_prepared || m_prepared->status() != Monero::PendingTransaction::Status_Ok) {
                if (m_prepared) m_wallet->disposeTransaction(m_prepared);
                m_prepared = nullptr;
                cancelPrepared();
            }
        }
        return true;
    } catch (const std::exception &e) {
        m_ready = false;
        setStatus(tr("Messenger unavailable: ") + QString::fromUtf8(e.what()));
        return false;
    }
}

bool Messenger::enabled() const
{
    return m_wallet->qmsStateExists();
}

bool Messenger::canEnable() const
{
    return m_wallet->qmsStateStorageAvailable() && m_wallet->qmsStrictTransportReady();
}

Messenger::~Messenger()
{
    // Keep the encrypted journal in wallet persistence for the next start.
    if (m_prepared) m_wallet->disposeTransaction(m_prepared);
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
    if ((latin.size() & 1) != 0) return {};
    const QByteArray result = QByteArray::fromHex(latin);
    return result.size() * 2 == latin.size() ? result : QByteArray{};
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

std::string Messenger::stateContext() const
{
    return qwertycoin::qms::wallet_state_context(genesis(), m_wallet->mainAddress());
}

std::string Messenger::legacyStateContext() const
{
    const auto network = genesis();
    return std::string("QWC-QMS2-GUI|")
        + hex(network.data(), network.size()).toStdString()
        + "|" + m_wallet->mainAddress();
}

void Messenger::ensureIdentity()
{
    if (m_crypto && !m_contactPackage.isEmpty()) return;
    m_crypto.reset(new qwertycoin::qms::crypto_backend(
        qwertycoin::qms::crypto_backend::create()));
    const auto prepared = m_crypto->prepare_contact_package(genesis());
    m_crypto.reset(new qwertycoin::qms::crypto_backend(prepared.next_state));
    m_invitationId = prepared.invitation_id;
    m_fingerprint = prepared.fingerprint;
    m_contactPackage = bytes(prepared.package);
    if (!save()) throw std::runtime_error("cannot persist new QMS2 identity");
}

void Messenger::load()
{
    std::string plaintext;
    bool migratedLegacyContext = false;
    if (!m_wallet->loadQmsState(plaintext, stateContext())) {
        const std::string neutralError = m_wallet->errorString();
        if (!m_wallet->loadQmsState(plaintext, legacyStateContext()))
            throw std::runtime_error(neutralError);
        migratedLegacyContext = !plaintext.empty();
    }
    const QByteArray raw = QByteArray::fromStdString(plaintext);
    if (raw.isEmpty()) return;
    const QJsonDocument document = QJsonDocument::fromJson(raw);
    if (!document.isObject()) throw std::runtime_error("invalid QMS2 wallet state");
    const QJsonObject root = document.object();
    if (root.value("profile").toInt() != qwertycoin::qms::CRYPTO_PROFILE_TRIPLE_RATCHET)
        throw std::runtime_error("unsupported or legacy QMS state profile");
    const QByteArray cryptoState = QByteArray::fromBase64(
        root.value("cryptoState").toString().toLatin1());
    m_contactPackage = QByteArray::fromBase64(
        root.value("contactPackage").toString().toLatin1());
    const QByteArray invitation = QByteArray::fromHex(
        root.value("invitationId").toString().toLatin1());
    const QByteArray fingerprint = QByteArray::fromHex(
        root.value("fingerprint").toString().toLatin1());
    if (cryptoState.isEmpty() || m_contactPackage.isEmpty()
        || size_t(invitation.size()) != m_invitationId.size()
        || size_t(fingerprint.size()) != m_fingerprint.size())
        throw std::runtime_error("incomplete QMS2 wallet identity");
    std::memcpy(m_invitationId.data(), invitation.constData(), m_invitationId.size());
    std::memcpy(m_fingerprint.data(), fingerprint.constData(), m_fingerprint.size());
    m_crypto.reset(new qwertycoin::qms::crypto_backend(bytes(cryptoState)));
    m_contacts = root.value("contacts").toArray();
    m_historyEnabled = root.value("historyEnabled").toBool(false);
    if (m_historyEnabled) m_messages = root.value("messages").toArray();
    m_incomplete = root.value("incomplete").toObject();
    m_seenMessages = root.value("seenMessages").toArray();
    m_preparedJournal = QByteArray::fromBase64(root.value("preparedJournal").toString().toLatin1());
    m_preparedMessageId = root.value("preparedMessageId").toString();
    m_preparedContactFingerprint = root.value("preparedContactFingerprint").toString();
    if (!m_preparedMessageId.isEmpty() && m_preparedContactFingerprint.isEmpty()) {
        for (const auto value : m_messages) {
            const QJsonObject message = value.toObject();
            if (message.value("id").toString() == m_preparedMessageId) {
                m_preparedContactFingerprint = message.value("contact").toString();
                break;
            }
        }
    }
    if (migratedLegacyContext && !save())
        throw std::runtime_error("cannot migrate legacy GUI messenger state context");
}

bool Messenger::save()
{
    if (!m_crypto) return false;
    QJsonObject root;
    root["profile"] = int(qwertycoin::qms::CRYPTO_PROFILE_TRIPLE_RATCHET);
    root["cryptoState"] = QString::fromLatin1(bytes(m_crypto->state()).toBase64());
    root["contactPackage"] = QString::fromLatin1(m_contactPackage.toBase64());
    root["invitationId"] = hex(m_invitationId.data(), m_invitationId.size());
    root["fingerprint"] = hex(m_fingerprint.data(), m_fingerprint.size());
    root["contacts"] = m_contacts;
    root["historyEnabled"] = m_historyEnabled;
    if (m_historyEnabled) root["messages"] = m_messages;
    root["incomplete"] = m_incomplete;
    root["seenMessages"] = m_seenMessages;
    root["preparedJournal"] = QString::fromLatin1(m_preparedJournal.toBase64());
    root["preparedMessageId"] = m_preparedMessageId;
    root["preparedContactFingerprint"] = m_preparedContactFingerprint;
    const bool stored = m_wallet->storeQmsState(
        QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString(), stateContext());
    if (!stored) {
        setStatus(tr("Messenger state could not be stored: ")
            + QString::fromStdString(m_wallet->errorString()));
        return false;
    }
    emit stateChanged();
    return true;
}

QString Messenger::ownInvitation() const
{
    return QString::fromLatin1(m_contactPackage.toHex());
}

QString Messenger::ownFingerprint() const
{
    return hex(m_fingerprint.data(), m_fingerprint.size());
}

QVariantList Messenger::contacts() const
{
    QVariantList result;
    for (const auto value : m_contacts) {
        const QJsonObject contact = value.toObject();
        if (!contact.value("removed").toBool(false))
            result.push_back(contact.toVariantMap());
    }
    return result;
}

QVariantList Messenger::messages() const
{
    QVariantList result; for (const auto value : m_messages) result.push_back(value.toObject().toVariantMap()); return result;
}

int Messenger::preparedTransactionCount() const { return m_prepared ? int(m_prepared->txCount()) : 0; }
quint64 Messenger::preparedFee() const { return m_prepared ? m_prepared->fee() : 0; }
bool Messenger::strictTransportReady() const { return m_wallet->qmsStrictTransportReady(); }

void Messenger::setStatus(const QString &value)
{
    if (m_status == value) return; m_status = value; emit statusChanged();
}

bool Messenger::importInvitation(const QString &label, const QString &encodedHex)
{
    try {
        if (!m_ready || !m_crypto) throw std::runtime_error("messenger is not ready");
        const QString normalizedLabel = label.trimmed();
        if (normalizedLabel.isEmpty()) throw std::runtime_error("contact name is required");
        const QByteArray raw = unhex(encodedHex.trimmed());
        if (raw.isEmpty()) throw std::runtime_error("invitation is not canonical hexadecimal");
        const auto prepared = m_crypto->prepare_import_contact(
            m_invitationId, bytes(raw), quint64(QDateTime::currentSecsSinceEpoch()));
        const QString fingerprint = hex(prepared.fingerprint.data(), prepared.fingerprint.size());
        if (fingerprint == ownFingerprint())
            throw std::runtime_error("cannot import this wallet's own messenger invitation");
        for (int i = 0; i < m_contacts.size(); ++i) {
            QJsonObject existing = m_contacts[i].toObject();
            if (existing.value("fingerprint").toString() != fingerprint) continue;
            if (!existing.value("removed").toBool(false))
                throw std::runtime_error("contact fingerprint is already present");
            const QJsonArray previous = m_contacts;
            existing["label"] = normalizedLabel;
            existing["removed"] = false;
            m_contacts[i] = existing;
            if (!save()) { m_contacts = previous; return false; }
            setStatus(tr("Contact restored; verify the fingerprint out of band."));
            return true;
        }

        QJsonObject contact;
        contact["label"] = normalizedLabel;
        contact["fingerprint"] = fingerprint;
        contact["contactId"] = QString::fromStdString(prepared.contact_id);
        contact["package"] = QString::fromLatin1(raw.toBase64());
        contact["confirmed"] = true;
        contact["removed"] = false;
        const QJsonArray previousContacts = m_contacts;
        const auto previousState = m_crypto->state();
        m_contacts.push_back(contact);
        m_crypto.reset(new qwertycoin::qms::crypto_backend(prepared.next_state));
        if (!save()) {
            m_contacts = previousContacts;
            m_crypto.reset(new qwertycoin::qms::crypto_backend(previousState));
            return false;
        }
        setStatus(tr("Contact imported; verify the fingerprint out of band."));
        return true;
    } catch (const std::exception &e) {
        setStatus(tr("Invitation rejected: ") + QString::fromUtf8(e.what()));
        return false;
    }
}

bool Messenger::renameContact(const QString &fingerprint, const QString &label)
{
    if (!m_ready) { setStatus(tr("Messenger is not ready.")); return false; }
    const QString normalized = label.trimmed();
    if (normalized.isEmpty()) { setStatus(tr("Contact name cannot be empty.")); return false; }
    const QJsonArray previousContacts = m_contacts;
    const QJsonArray previousMessages = m_messages;
    bool found = false;
    for (int i = 0; i < m_contacts.size(); ++i) {
        QJsonObject contact = m_contacts[i].toObject();
        if (contact.value("fingerprint").toString() != fingerprint) continue;
        contact["label"] = normalized; m_contacts[i] = contact; found = true; break;
    }
    if (!found) { setStatus(tr("Contact not found.")); return false; }
    for (int i = 0; i < m_messages.size(); ++i) {
        QJsonObject message = m_messages[i].toObject();
        if (message.value("contact").toString() != fingerprint) continue;
        message["label"] = normalized; m_messages[i] = message;
    }
    if (!save()) { m_contacts = previousContacts; m_messages = previousMessages; return false; }
    setStatus(tr("Contact renamed.")); return true;
}

bool Messenger::removeContact(const QString &fingerprint)
{
    if (!m_ready) { setStatus(tr("Messenger is not ready.")); return false; }
    if (!m_preparedContactFingerprint.isEmpty() && m_preparedContactFingerprint == fingerprint) {
        setStatus(tr("Cancel or send the prepared message before removing this contact.")); return false;
    }
    for (int i = 0; i < m_contacts.size(); ++i) {
        if (m_contacts[i].toObject().value("fingerprint").toString() != fingerprint) continue;
        const QJsonArray previous = m_contacts;
        QJsonObject contact = m_contacts[i].toObject();
        contact["removed"] = true;
        m_contacts[i] = contact;
        if (!save()) { m_contacts = previous; return false; }
        setStatus(tr("Contact removed. Existing local message history was retained.")); return true;
    }
    setStatus(tr("Contact not found.")); return false;
}

bool Messenger::prepare(const QString &contactFingerprint, const QString &text)
{
    cancelPrepared();
    try {
        if (!m_ready || !m_crypto) throw std::runtime_error("messenger is not ready");
        if (!strictTransportReady())
            throw std::runtime_error("QMS2 requires a SOCKS proxy and Tor v3 onion daemon");
        const QByteArray utf8 = text.toUtf8();
        if (utf8.isEmpty()) throw std::runtime_error("message is empty");
        if (utf8.size() > int(qwertycoin::qms::MAX_TEXT_BYTES))
            throw std::runtime_error("message exceeds 4096 UTF-8 bytes");
        QString label;
        std::string contactId;
        bool found = false;
        for (const auto value : m_contacts) {
            const QJsonObject contact = value.toObject();
            if (contact.value("fingerprint").toString() != contactFingerprint
                || contact.value("removed").toBool(false)) continue;
            contactId = contact.value("contactId").toString().toStdString();
            label = contact.value("label").toString();
            found = !contactId.empty();
            break;
        }
        if (!found) throw std::runtime_error("unknown messenger contact");
        const auto preparedSend = m_crypto->prepare_send_text(
            contactId, utf8.toStdString(), quint64(QDateTime::currentSecsSinceEpoch()));
        qwertycoin::qms::bytes inner;
        inner.reserve(1 + preparedSend.ciphertext.data.size());
        inner.push_back(preparedSend.ciphertext.message_type);
        inner.insert(inner.end(), preparedSend.ciphertext.data.begin(),
                     preparedSend.ciphertext.data.end());
        const auto context = m_crypto->transport_context(contactId, true);
        const auto envelope = qwertycoin::qms::seal_outer_envelope(
            context, preparedSend.message_id, inner);
        const auto fragments = qwertycoin::qms::fragment_envelope(
            context, preparedSend.message_id, envelope);
        std::vector<std::vector<uint8_t>> extras;
        for (const auto &fragment : fragments) { std::vector<uint8_t> extra; if (!qwertycoin::qms::append_carrier_nonces(extra, fragment)) throw std::runtime_error("fragment does not fit unchanged tx_extra limit"); extras.push_back(std::move(extra)); }
        m_prepared = m_wallet->createQmsCarrierTransactions(extras, 1, m_wallet->defaultMixin());
        if (!m_prepared || m_prepared->status() != Monero::PendingTransaction::Status_Ok)
            throw std::runtime_error(m_prepared ? m_prepared->errorString() : "wallet returned no transaction plan");
        m_preparedMessageId = hex(preparedSend.message_id.data(), preparedSend.message_id.size());
        m_preparedContactFingerprint = contactFingerprint;
        m_preparedJournal = QByteArray::fromStdString(m_prepared->qmsJournalData());
        if (m_preparedJournal.isEmpty()) throw std::runtime_error("wallet could not create encrypted QMS journal");
        QJsonObject message; message["id"] = m_preparedMessageId; message["contact"] = contactFingerprint;
        message["label"] = label; message["text"] = text; message["direction"] = "out";
        message["status"] = DOMAIN_STATUS_PREPARED; message["transactions"] = int(m_prepared->txCount());
        message["fee"] = QString::number(m_prepared->fee());
        message["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        const auto previousState = m_crypto->state();
        const QJsonArray previousMessages = m_messages;
        m_crypto.reset(new qwertycoin::qms::crypto_backend(preparedSend.next_state));
        m_messages.push_back(message);
        if (!save()) {
            m_crypto.reset(new qwertycoin::qms::crypto_backend(previousState));
            m_messages = previousMessages;
            m_wallet->disposeTransaction(m_prepared);
            m_prepared = nullptr;
            m_preparedMessageId.clear();
            m_preparedContactFingerprint.clear();
            m_preparedJournal.clear();
            emit planChanged();
            return false;
        }
        setStatus(tr("Encrypted and prepared. Review transaction count and total fee, then send explicitly.")); emit planChanged(); return true;
    } catch (const std::exception &e) {
        cancelPrepared();
        setStatus(tr("Preparation failed: ") + QString::fromUtf8(e.what()));
        return false;
    }
}

bool Messenger::commitPrepared()
{
    if (!m_prepared) { setStatus(tr("No prepared message.")); return false; }
    if (m_checkpointPending) {
        if (!save()) {
            setStatus(tr("The previous carrier checkpoint is still not durable. No carrier was submitted."));
            emit planChanged();
            return false;
        }
        m_checkpointPending = false;
        if (m_prepared->txCount() == 0) {
            m_wallet->disposeTransaction(m_prepared); m_prepared = nullptr;
            setStatus(tr("Encrypted message transactions submitted and completion persisted."));
            emit planChanged();
            return true;
        }
        setStatus(tr("The carrier checkpoint is now durable. Review the remaining transaction count, then resume sending explicitly."));
        emit planChanged();
        return true;
    }
    while (m_prepared->txCount() != 0) {
        if (!m_prepared->commitQmsNext()) {
            setStatus(tr("Carrier submission stopped; the remaining encrypted transactions are still recoverable: ")
                + QString::fromStdString(m_prepared->errorString()));
            emit planChanged();
            return false;
        }

        for (int i = 0; i < m_messages.size(); ++i) {
            QJsonObject message = m_messages[i].toObject();
            if (message.value("id").toString() != m_preparedMessageId) continue;
            message["submittedTransactions"] =
                message.value("transactions").toInt() - int(m_prepared->txCount());
            m_messages[i] = message;
            break;
        }

        if (m_prepared->txCount() != 0) {
            m_preparedJournal = QByteArray::fromStdString(m_prepared->qmsJournalData());
            m_checkpointPending = true;
            if (m_preparedJournal.isEmpty() || !save()) {
                setStatus(tr("Carrier accepted, but the reduced recovery journal could not be persisted. "
                    "No further carrier was submitted."));
                emit planChanged();
                return false;
            }
            m_checkpointPending = false;
        }
    }

    for (int i = 0; i < m_messages.size(); ++i) {
        QJsonObject message = m_messages[i].toObject();
        if (message.value("id").toString() != m_preparedMessageId) continue;
        message["status"] = "sent";
        m_messages[i] = message;
        break;
    }
    setStatus(tr("Encrypted message transactions submitted."));
    m_preparedMessageId.clear(); m_preparedContactFingerprint.clear(); m_preparedJournal.clear();
    if (!m_historyEnabled) {
        for (int i = m_messages.size() - 1; i >= 0; --i)
            if (m_messages[i].toObject().value("direction").toString() == "out"
                && m_messages[i].toObject().value("status").toString() != DOMAIN_STATUS_PREPARED)
                m_messages.removeAt(i);
    }
    m_checkpointPending = true;
    if (!save()) {
        setStatus(tr("All carriers were submitted, but local completion could not be persisted. "
            "Retry Send to persist completion; no carrier will be rebroadcast."));
        emit planChanged();
        return false;
    }
    m_checkpointPending = false;
    m_wallet->disposeTransaction(m_prepared); m_prepared = nullptr;
    emit planChanged();
    return true;
}

void Messenger::cancelPrepared()
{
    if (m_checkpointPending && !save()) {
        setStatus(tr("The carrier checkpoint is not durable yet; cancellation was refused."));
        emit planChanged();
        return;
    }
    if (m_checkpointPending) {
        m_checkpointPending = false;
        if (m_prepared && m_prepared->txCount() == 0) {
            m_wallet->disposeTransaction(m_prepared); m_prepared = nullptr;
            setStatus(tr("Encrypted message transactions submitted and completion persisted."));
            emit planChanged();
            return;
        }
    }
    if (m_prepared) m_wallet->disposeTransaction(m_prepared);
    if (!m_preparedMessageId.isEmpty()) {
        for (int i = m_messages.size() - 1; i >= 0; --i) {
            const QJsonObject message = m_messages[i].toObject();
            if (message.value("id").toString() == m_preparedMessageId &&
                message.value("status").toString() == DOMAIN_STATUS_PREPARED)
                m_messages.removeAt(i);
        }
    }
    m_prepared = nullptr; m_preparedMessageId.clear(); m_preparedContactFingerprint.clear(); m_preparedJournal.clear();
    if (m_crypto) save();
    emit planChanged();
}

void Messenger::ingestCarrier(quint64 height, const QString &blockHash, const QString &txId, const QString &extraHex)
{
    try {
        if (!m_ready || !m_crypto) return;
        if (!strictTransportReady()) {
            setStatus(tr("Messenger sync blocked: configure a SOCKS proxy and Tor v3 onion daemon."));
            return;
        }
        const auto fragments = qwertycoin::qms::extract_carrier_fragments(bytes(unhex(extraHex)));
        if (fragments.size() != 1
            || fragments[0].profile != qwertycoin::qms::CRYPTO_PROFILE_TRIPLE_RATCHET)
            return;
        const auto &fragment = fragments[0];
        const QString id = hex(fragment.message_id.data(), fragment.message_id.size());
        QString contactId;
        QString contactFingerprint;
        QString contactLabel;
        qwertycoin::qms::envelope_context context;
        bool matched = false;
        for (const auto value : m_contacts) {
            const QJsonObject contact = value.toObject();
            const std::string candidateId = contact.value("contactId").toString().toStdString();
            if (candidateId.empty()) continue;
            try {
                const auto candidate = m_crypto->transport_context(candidateId, false);
                if (!qwertycoin::qms::verify_envelope_fragment(candidate, fragment)) continue;
                contactId = QString::fromStdString(candidateId);
                contactFingerprint = contact.value("fingerprint").toString();
                contactLabel = contact.value("label").toString();
                context = candidate;
                matched = true;
                break;
            } catch (...) {}
        }
        if (!matched) return;
        const QString seenKey = contactId + ":" + id;
        for (const auto value : m_seenMessages)
            if (value.toString() == seenKey) return;

        const QString incompleteKey = seenKey;
        const QJsonObject previousIncomplete = m_incomplete;
        QJsonObject incomplete = m_incomplete.value(incompleteKey).toObject();
        if (!incomplete.isEmpty() && incomplete.value("count").toInt() != fragment.count) {
            m_incomplete.remove(incompleteKey);
            if (!save()) m_incomplete = previousIncomplete;
            return;
        }
        QJsonArray parts = incomplete.value("parts").toArray();
        const QString encoded = QString::fromLatin1(bytes(qwertycoin::qms::encode_fragment(fragment)).toBase64());
        for (const auto value : parts) if (value.toObject().value("index").toInt() == fragment.index) {
            if (value.toObject().value("data").toString() != encoded) {
                m_incomplete.remove(incompleteKey);
                if (!save()) m_incomplete = previousIncomplete;
            }
            return;
        }
        if (!m_incomplete.contains(incompleteKey)) {
            if (m_incomplete.size() >= MAX_INCOMPLETE_MESSAGES) return;
            int perContact = 0;
            for (auto i = m_incomplete.begin(); i != m_incomplete.end(); ++i)
                if (i.value().toObject().value("contactId").toString() == contactId)
                    ++perContact;
            if (perContact >= MAX_INCOMPLETE_PER_CONTACT) return;
        }
        qint64 storedBytes = 0;
        for (auto i = m_incomplete.begin(); i != m_incomplete.end(); ++i)
            for (const auto partValue : i.value().toObject().value("parts").toArray())
                storedBytes += QByteArray::fromBase64(
                    partValue.toObject().value("data").toString().toLatin1()).size();
        if (storedBytes + QByteArray::fromBase64(encoded.toLatin1()).size()
            > MAX_REASSEMBLY_BYTES) return;
        QJsonObject part; part["index"] = fragment.index; part["data"] = encoded; parts.push_back(part);
        incomplete["contactId"] = contactId;
        incomplete["fingerprint"] = contactFingerprint;
        incomplete["count"] = fragment.count;
        incomplete["parts"] = parts;
        m_incomplete[incompleteKey] = incomplete;
        if (parts.size() != fragment.count) {
            if (!save()) m_incomplete = previousIncomplete;
            return;
        }
        std::vector<qwertycoin::qms::fragment> complete; for (const auto value : parts) complete.push_back(qwertycoin::qms::decode_fragment(bytes(QByteArray::fromBase64(value.toObject().value("data").toString().toLatin1()))));
        const auto envelope = qwertycoin::qms::reassemble(complete);
        const auto opened = qwertycoin::qms::open_outer_envelope(
            context, fragment.message_id, envelope);
        if (opened.size() < 2) throw std::runtime_error("empty QMS2 inner ciphertext");
        qwertycoin::qms::ratchet_ciphertext ciphertext;
        ciphertext.message_type = opened.front();
        ciphertext.data.assign(opened.begin() + 1, opened.end());
        const auto received = m_crypto->prepare_receive_text(
            contactId.toStdString(), ciphertext);
        if (received.message_id != fragment.message_id)
            throw std::runtime_error("QMS2 message identifier mismatch");

        const auto previousState = m_crypto->state();
        const QJsonArray previousSeen = m_seenMessages;
        const QJsonArray previousMessages = m_messages;
        m_crypto.reset(new qwertycoin::qms::crypto_backend(received.next_state));
        m_incomplete.remove(incompleteKey);
        m_seenMessages.push_back(seenKey);
        while (m_seenMessages.size() > MAX_SEEN_MESSAGES) m_seenMessages.removeAt(0);
        QJsonObject message; message["id"] = id; message["contact"] = contactFingerprint;
        message["label"] = contactLabel;
        message["text"] = QString::fromUtf8(received.text.data(), int(received.text.size()));
        message["direction"] = "in"; message["status"] = "confirmed";
        message["height"] = QString::number(height); message["blockHash"] = blockHash;
        message["txId"] = txId;
        message["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        m_messages.push_back(message);
        if (!save()) {
            m_crypto.reset(new qwertycoin::qms::crypto_backend(previousState));
            m_incomplete = previousIncomplete;
            m_seenMessages = previousSeen;
            m_messages = previousMessages;
        }
    } catch (...) { /* unauthenticated malformed chain data is ignored */ }
}

void Messenger::handleReorg(quint64 height, quint64)
{
    bool changed = false; for (int i = 0; i < m_messages.size(); ++i) { QJsonObject message = m_messages[i].toObject();
        if (message.value("direction") == "in" && message.value("height").toString().toULongLong() >= height) { message["status"] = "chain proof lost"; m_messages[i] = message; changed = true; } }
    if (changed) save();
}

void Messenger::setHistoryEnabled(bool enabled)
{
    if (m_historyEnabled == enabled) return;
    const bool previous = m_historyEnabled;
    m_historyEnabled = enabled;
    if (!save()) { m_historyEnabled = previous; return; }
    emit historyEnabledChanged();
}

void Messenger::clearHistory()
{
    const QJsonArray previous = m_messages;
    m_messages = QJsonArray();
    if (!save()) { m_messages = previous; return; }
    setStatus(tr("Local messenger history cleared. Blockchain carrier data is unchanged."));
}
