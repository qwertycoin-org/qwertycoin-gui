#include "Messenger.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QDateTime>
#include <QVariantMap>
#include <cstring>
#include <sodium.h>

#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "string_tools.h"

namespace
{
    const char *STATE_KEY = "qms/state/v1";
    const char *DOMAIN_STATUS_PREPARED = "prepared";
}

Messenger::Messenger(Monero::Wallet *wallet, QObject *parent)
    : QObject(parent), m_wallet(wallet)
{
    load();
    ensureIdentity();
    if (!m_preparedJournal.isEmpty()) {
        m_prepared = m_wallet->restoreQmsCarrierTransactions(m_preparedJournal.toStdString());
        if (!m_prepared || m_prepared->status() != Monero::PendingTransaction::Status_Ok) {
            if (m_prepared) m_wallet->disposeTransaction(m_prepared);
            m_prepared = nullptr; m_preparedJournal.clear(); m_preparedMessageId.clear();
            m_preparedContactFingerprint.clear(); save();
        }
    }
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

void Messenger::ensureIdentity()
{
    if (m_invitation.version == qwertycoin::qms::WIRE_VERSION &&
        qwertycoin::qms::verify_invitation(m_invitation)) return;
    m_identity = qwertycoin::qms::generate_identity();
    m_invitation = qwertycoin::qms::create_invitation(m_identity, genesis());
    save();
}

void Messenger::load()
{
    const QByteArray raw = QByteArray::fromStdString(m_wallet->getCacheAttribute(STATE_KEY));
    const QJsonDocument document = QJsonDocument::fromJson(raw);
    if (!document.isObject()) return;
    const QJsonObject root = document.object();
    auto copy = [](const QString &encoded, uint8_t *destination, size_t size) {
        const QByteArray decoded = QByteArray::fromBase64(encoded.toLatin1());
        if (size_t(decoded.size()) != size) return false;
        std::memcpy(destination, decoded.constData(), size); return true;
    };
    const QJsonObject identity = root.value("identity").toObject();
    bool valid = copy(identity.value("boxPublic").toString(), m_identity.box_public.data(), m_identity.box_public.size()) &&
        copy(identity.value("boxSecret").toString(), m_identity.box_secret.data(), m_identity.box_secret.size()) &&
        copy(identity.value("signPublic").toString(), m_identity.sign_public.data(), m_identity.sign_public.size()) &&
        copy(identity.value("signSecret").toString(), m_identity.sign_secret.data(), m_identity.sign_secret.size());
    if (valid) {
        try {
            m_invitation = qwertycoin::qms::decode_invitation(bytes(QByteArray::fromBase64(root.value("invitation").toString().toLatin1())));
            valid = m_invitation.genesis == genesis();
        } catch (...) { valid = false; }
    }
    if (!valid) m_invitation = {};
    m_contacts = root.value("contacts").toArray();
    m_messages = root.value("messages").toArray();
    m_incomplete = root.value("incomplete").toObject();
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
}

void Messenger::save()
{
    auto b64 = [](const uint8_t *data, size_t size) {
        return QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(data), int(size)).toBase64());
    };
    QJsonObject identity;
    identity["boxPublic"] = b64(m_identity.box_public.data(), m_identity.box_public.size());
    identity["boxSecret"] = b64(m_identity.box_secret.data(), m_identity.box_secret.size());
    identity["signPublic"] = b64(m_identity.sign_public.data(), m_identity.sign_public.size());
    identity["signSecret"] = b64(m_identity.sign_secret.data(), m_identity.sign_secret.size());
    QJsonObject root;
    root["identity"] = identity;
    root["invitation"] = QString::fromLatin1(bytes(qwertycoin::qms::encode_invitation(m_invitation)).toBase64());
    root["contacts"] = m_contacts;
    root["messages"] = m_messages;
    root["incomplete"] = m_incomplete;
    root["preparedJournal"] = QString::fromLatin1(m_preparedJournal.toBase64());
    root["preparedMessageId"] = m_preparedMessageId;
    root["preparedContactFingerprint"] = m_preparedContactFingerprint;
    // Wallet cache attributes are covered by the wallet's encrypted cache persistence.
    m_wallet->setCacheAttribute(STATE_KEY, QJsonDocument(root).toJson(QJsonDocument::Compact).toStdString());
    emit stateChanged();
}

qwertycoin::qms::invitation Messenger::ownInvitationValue() const { return m_invitation; }

QString Messenger::ownInvitation() const
{
    return QString::fromStdString(qwertycoin::qms::hex(qwertycoin::qms::encode_invitation(m_invitation)));
}

QString Messenger::ownFingerprint() const
{
    const auto value = qwertycoin::qms::fingerprint(m_identity.box_public, m_identity.sign_public);
    return hex(value.data(), value.size());
}

QVariantList Messenger::contacts() const
{
    QVariantList result; for (const auto value : m_contacts) result.push_back(value.toObject().toVariantMap()); return result;
}

QVariantList Messenger::messages() const
{
    QVariantList result; for (const auto value : m_messages) result.push_back(value.toObject().toVariantMap()); return result;
}

int Messenger::preparedTransactionCount() const { return m_prepared ? int(m_prepared->txCount()) : 0; }
quint64 Messenger::preparedFee() const { return m_prepared ? m_prepared->fee() : 0; }

void Messenger::setStatus(const QString &value)
{
    if (m_status == value) return; m_status = value; emit statusChanged();
}

bool Messenger::importInvitation(const QString &label, const QString &encodedHex)
{
    try {
        const QString normalizedLabel = label.trimmed();
        if (normalizedLabel.isEmpty()) throw std::runtime_error("contact name is required");
        const QByteArray raw = unhex(encodedHex.trimmed());
        if (raw.isEmpty()) throw std::runtime_error("invitation is not canonical hexadecimal");
        const auto invitation = qwertycoin::qms::decode_invitation(bytes(raw));
        if (invitation.genesis != genesis()) throw std::runtime_error("invitation belongs to another network");
        const auto fp = qwertycoin::qms::fingerprint(invitation.box_public, invitation.sign_public);
        const QString fingerprint = hex(fp.data(), fp.size());
        if (fingerprint == ownFingerprint())
            throw std::runtime_error("cannot import this wallet's own messenger invitation");
        for (const auto value : m_contacts)
            if (value.toObject().value("fingerprint").toString() == fingerprint)
                throw std::runtime_error("contact fingerprint is already present");
        QJsonObject contact; contact["label"] = normalizedLabel; contact["fingerprint"] = fingerprint;
        contact["invitation"] = encodedHex.trimmed().toLower(); contact["confirmed"] = true;
        m_contacts.push_back(contact); save(); setStatus(tr("Contact imported; verify the fingerprint out of band.")); return true;
    } catch (const std::exception &e) { setStatus(tr("Invitation rejected: ") + e.what()); return false; }
}

bool Messenger::renameContact(const QString &fingerprint, const QString &label)
{
    const QString normalized = label.trimmed();
    if (normalized.isEmpty()) { setStatus(tr("Contact name cannot be empty.")); return false; }
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
    save(); setStatus(tr("Contact renamed.")); return true;
}

bool Messenger::removeContact(const QString &fingerprint)
{
    if (!m_preparedContactFingerprint.isEmpty() && m_preparedContactFingerprint == fingerprint) {
        setStatus(tr("Cancel or send the prepared message before removing this contact.")); return false;
    }
    for (int i = 0; i < m_contacts.size(); ++i) {
        if (m_contacts[i].toObject().value("fingerprint").toString() != fingerprint) continue;
        m_contacts.removeAt(i); save();
        setStatus(tr("Contact removed. Existing local message history was retained.")); return true;
    }
    setStatus(tr("Contact not found.")); return false;
}

bool Messenger::prepare(const QString &contactFingerprint, const QString &text)
{
    cancelPrepared();
    try {
        qwertycoin::qms::invitation recipient; QString label; bool found = false;
        for (const auto value : m_contacts) { const QJsonObject contact = value.toObject(); if (contact.value("fingerprint").toString() == contactFingerprint) {
            recipient = qwertycoin::qms::decode_invitation(bytes(unhex(contact.value("invitation").toString()))); label = contact.value("label").toString(); found = true; break; } }
        if (!found) throw std::runtime_error("unknown messenger contact");
        qwertycoin::qms::id16 id{}; randombytes_buf(id.data(), id.size());
        const std::string utf8 = text.toUtf8().toStdString();
        const auto ciphertext = qwertycoin::qms::seal_text(m_identity, recipient, genesis(), id, utf8);
        const auto fragments = qwertycoin::qms::fragment_ciphertext(recipient, genesis(), id, ciphertext);
        std::vector<std::vector<uint8_t>> extras;
        for (const auto &fragment : fragments) { std::vector<uint8_t> extra; if (!qwertycoin::qms::append_carrier_nonces(extra, fragment)) throw std::runtime_error("fragment does not fit unchanged tx_extra limit"); extras.push_back(std::move(extra)); }
        m_prepared = m_wallet->createQmsCarrierTransactions(extras, 1, m_wallet->defaultMixin());
        if (!m_prepared || m_prepared->status() != Monero::PendingTransaction::Status_Ok)
            throw std::runtime_error(m_prepared ? m_prepared->errorString() : "wallet returned no transaction plan");
        m_preparedMessageId = hex(id.data(), id.size());
        m_preparedContactFingerprint = contactFingerprint;
        m_preparedJournal = QByteArray::fromStdString(m_prepared->qmsJournalData());
        if (m_preparedJournal.isEmpty()) throw std::runtime_error("wallet could not create encrypted QMS journal");
        QJsonObject message; message["id"] = m_preparedMessageId; message["contact"] = contactFingerprint;
        message["label"] = label; message["text"] = text; message["direction"] = "out";
        message["status"] = DOMAIN_STATUS_PREPARED; message["transactions"] = int(m_prepared->txCount());
        message["fee"] = QString::number(m_prepared->fee());
        message["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate); m_messages.push_back(message); save();
        setStatus(tr("Encrypted and prepared. Review transaction count and total fee, then send explicitly.")); emit planChanged(); return true;
    } catch (const std::exception &e) { cancelPrepared(); setStatus(tr("Preparation failed: ") + e.what()); return false; }
}

bool Messenger::commitPrepared()
{
    if (!m_prepared) { setStatus(tr("No prepared message.")); return false; }
    const bool ok = m_prepared->commit();
    for (int i = 0; i < m_messages.size(); ++i) { QJsonObject message = m_messages[i].toObject(); if (message.value("id").toString() == m_preparedMessageId) {
        message["status"] = ok ? "sent" : "send failed"; m_messages[i] = message; break; } }
    if (!ok) setStatus(QString::fromStdString(m_prepared->errorString())); else setStatus(tr("Encrypted message transactions submitted."));
    m_wallet->disposeTransaction(m_prepared); m_prepared = nullptr; m_preparedMessageId.clear(); m_preparedContactFingerprint.clear(); m_preparedJournal.clear(); save(); emit planChanged(); return ok;
}

void Messenger::cancelPrepared()
{
    if (m_prepared) m_wallet->disposeTransaction(m_prepared);
    m_prepared = nullptr; m_preparedMessageId.clear(); m_preparedContactFingerprint.clear(); m_preparedJournal.clear(); save(); emit planChanged();
}

void Messenger::ingestCarrier(quint64 height, const QString &blockHash, const QString &txId, const QString &extraHex)
{
    try {
        const auto fragments = qwertycoin::qms::extract_carrier_fragments(bytes(unhex(extraHex)));
        if (fragments.size() != 1 || !qwertycoin::qms::verify_fragment(m_invitation, genesis(), fragments[0])) return;
        const auto &fragment = fragments[0]; const QString id = hex(fragment.message_id.data(), fragment.message_id.size());
        QJsonArray parts = m_incomplete.value(id).toArray();
        const QString encoded = QString::fromLatin1(bytes(qwertycoin::qms::encode_fragment(fragment)).toBase64());
        for (const auto value : parts) if (value.toObject().value("index").toInt() == fragment.index) {
            if (value.toObject().value("data").toString() != encoded) { m_incomplete.remove(id); save(); } return; }
        if (m_incomplete.size() >= 64 && !m_incomplete.contains(id)) return;
        QJsonObject part; part["index"] = fragment.index; part["data"] = encoded; parts.push_back(part); m_incomplete[id] = parts;
        if (parts.size() != fragment.count) { save(); return; }
        std::vector<qwertycoin::qms::fragment> complete; for (const auto value : parts) complete.push_back(qwertycoin::qms::decode_fragment(bytes(QByteArray::fromBase64(value.toObject().value("data").toString().toLatin1()))));
        const auto ciphertext = qwertycoin::qms::reassemble(complete);
        for (const auto value : m_contacts) { const QJsonObject contact = value.toObject(); try {
            const auto sender = qwertycoin::qms::decode_invitation(bytes(unhex(contact.value("invitation").toString())));
            const auto opened = qwertycoin::qms::open_text(m_identity, sender, m_invitation, genesis(), fragment.message_id, ciphertext);
            QJsonObject message; message["id"] = id; message["contact"] = contact.value("fingerprint"); message["label"] = contact.value("label");
            message["text"] = QString::fromUtf8(opened.text.data(), int(opened.text.size())); message["direction"] = "in"; message["status"] = "confirmed";
            message["height"] = QString::number(height); message["blockHash"] = blockHash; message["txId"] = txId;
            message["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate); m_messages.push_back(message); m_incomplete.remove(id); save(); return;
        } catch (...) {} }
        m_incomplete.remove(id); save();
    } catch (...) { /* unauthenticated malformed chain data is ignored */ }
}

void Messenger::handleReorg(quint64 height, quint64)
{
    bool changed = false; for (int i = 0; i < m_messages.size(); ++i) { QJsonObject message = m_messages[i].toObject();
        if (message.value("direction") == "in" && message.value("height").toString().toULongLong() >= height) { message["status"] = "chain proof lost"; m_messages[i] = message; changed = true; } }
    if (changed) save();
}
