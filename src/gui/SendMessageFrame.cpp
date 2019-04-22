// Copyright (c) 2011-2016 The Cryptonote developers
// Copyright (c) 2015-2016 XDN developers
// Copyright (c) 2018-2019, The Qwertycoin developers
//
// This file is part of Qwertycoin.
//
// Qwertycoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Qwertycoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Qwertycoin.  If not, see <http://www.gnu.org/licenses/>.

#include <QClipboard>

#include "SendMessageFrame.h"
#include "CurrencyAdapter.h"
#include "MainWindow.h"
#include "Message.h"
#include "MessageAddressFrame.h"
#include "MessagesModel.h"
#include "WalletAdapter.h"
#include "WalletEvents.h"

#include "ui_sendmessageframe.h"

namespace WalletGui {

Q_DECL_CONSTEXPR quint64 MESSAGE_AMOUNT = 100000;
Q_DECL_CONSTEXPR quint64 MESSAGE_CHAR_PRICE = 1000000;
Q_DECL_CONSTEXPR quint64 MINIMAL_MESSAGE_FEE = 100000000;
Q_DECL_CONSTEXPR int DEFAULT_MESSAGE_MIXIN = 2;

Q_DECL_CONSTEXPR quint32 MINUTE_SECONDS = 60;
Q_DECL_CONSTEXPR quint32 HOUR_SECONDS = 60 * MINUTE_SECONDS;
Q_DECL_CONSTEXPR int MIN_TTL = 5 * MINUTE_SECONDS;
Q_DECL_CONSTEXPR int MAX_TTL = 14 * HOUR_SECONDS;
Q_DECL_CONSTEXPR int TTL_STEP = 5 * MINUTE_SECONDS;

SendMessageFrame::SendMessageFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::SendMessageFrame) {
  m_ui->setupUi(this);
  m_ui->m_mixinSlider->setValue(DEFAULT_MESSAGE_MIXIN);
  m_ui->m_feeSpin->setMinimum(CurrencyAdapter::instance().formatAmount(MESSAGE_AMOUNT + MINIMAL_MESSAGE_FEE).toDouble());
  m_ui->m_feeSpin->setValue(m_ui->m_feeSpin->minimum());
  m_ui->m_ttlSlider->setVisible(false);
  m_ui->m_ttlLabel->setVisible(false);

  m_ui->m_ttlSlider->setMinimum(1);
  m_ui->m_ttlSlider->setMaximum(MAX_TTL / MIN_TTL);
  ttlValueChanged(m_ui->m_ttlSlider->value());
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSendMessageCompletedSignal, this, &SendMessageFrame::sendMessageCompleted,
    Qt::QueuedConnection);
  reset();
}

SendMessageFrame::~SendMessageFrame() {
}

void SendMessageFrame::setAddress(const QString& _address) {
  m_ui->m_addReplyToCheck->setChecked(true);
}

void SendMessageFrame::sendMessageCompleted(CryptoNote::TransactionId _transaction_id, bool _error, const QString& _error_text) {
  Q_UNUSED(_transaction_id);
  if (_error) {
    QCoreApplication::postEvent(
      &MainWindow::instance(),
      new ShowMessageEvent(_error_text, QtCriticalMsg));
  } else {
    reset();
  }
}

void SendMessageFrame::reset() {
  m_ui->m_mixinSlider->setValue(DEFAULT_MESSAGE_MIXIN);
  m_ui->m_feeSpin->setValue(MESSAGE_AMOUNT + MINIMAL_MESSAGE_FEE);
  m_ui->m_messageTextEdit->clear();
  for (MessageAddressFrame* addressFrame : m_addressFrames) {
    addressFrame->deleteLater();
  }

  addRecipientClicked();
}

QString SendMessageFrame::extractAddress(const QString& _addressString) const {
  QString address = _addressString;
  if (_addressString.contains('<')) {
    int startPos = _addressString.indexOf('<');
    int endPos = _addressString.indexOf('>');
    address = _addressString.mid(startPos + 1, endPos - startPos - 1);
  }

  return address;
}

void SendMessageFrame::recalculateFeeValue() {
  QString messageText = m_ui->m_messageTextEdit->toPlainText();
  quint32 messageSize = messageText.length();
  if (messageSize > 0) {
    --messageSize;
  }

  quint64 fee = MESSAGE_AMOUNT * m_addressFrames.size();
  if (m_ui->m_ttlCheck->checkState() == Qt::Unchecked) {
    fee += MINIMAL_MESSAGE_FEE + messageSize * MESSAGE_CHAR_PRICE;
  }

  m_ui->m_feeSpin->setMinimum(CurrencyAdapter::instance().formatAmount(fee).toDouble());

  m_ui->m_feeSpin->setValue(m_ui->m_feeSpin->minimum());
}

void SendMessageFrame::addRecipientClicked() {
  MessageAddressFrame* newAddress = new MessageAddressFrame(m_ui->m_messageAddressScrollArea);
  m_ui->m_addressesLayout->insertWidget(m_addressFrames.size(), newAddress);
  m_addressFrames.append(newAddress);
  if (m_addressFrames.size() == 1) {
    newAddress->disableRemoveButton(true);
  } else {
    m_addressFrames[0]->disableRemoveButton(false);
  }

  connect(newAddress, &MessageAddressFrame::destroyed, [this](QObject* _obj) {
    m_addressFrames.removeOne(static_cast<MessageAddressFrame*>(_obj));
    if (m_addressFrames.size() == 1) {
      m_addressFrames[0]->disableRemoveButton(true);
    }

    recalculateFeeValue();
  });

  recalculateFeeValue();
  m_ui->m_messageAddressScrollArea->setFixedHeight(3 * newAddress->height());
}

void SendMessageFrame::messageTextChanged() {
  recalculateFeeValue();
}

void SendMessageFrame::mixinValueChanged(int _value) {
  m_ui->m_mixinEdit->setText(QString::number(_value));
}

void SendMessageFrame::sendClicked() {
  if (!WalletAdapter::instance().isOpen()) {
    return;
  }
  QVector<CryptoNote::WalletLegacyTransfer> transfers;
  QVector<CryptoNote::TransactionMessage> messages;
  QString messageString = m_ui->m_messageTextEdit->toPlainText();
  QString messageSender = "";
  if (m_ui->m_addReplyToCheck->isChecked()) {
    MessageHeader header;
    header.append(qMakePair(QString(MessagesModel::HEADER_REPLY_TO_KEY), WalletAdapter::instance().getAddress()));
    messageString = Message::makeTextMessage(messageString, header);
    messageSender = WalletAdapter::instance().getAddress();
  }

  transfers.reserve(m_addressFrames.size());
  for (MessageAddressFrame* addressFrame : m_addressFrames) {
    QString address = extractAddress(addressFrame->getAddress());
    if (!CurrencyAdapter::instance().validateAddress(address)) {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid recipient address"), QtCriticalMsg));
      return;
    }

    transfers.append({address.toStdString(), MESSAGE_AMOUNT});
    messages.append({messageString.toStdString(), address.toStdString()});
  }

  quint64 ttl = 0;
  if (m_ui->m_ttlCheck->checkState() == Qt::Checked) {
    ttl = QDateTime::currentDateTimeUtc().toTime_t() + m_ui->m_ttlSlider->value() * MIN_TTL;
  }

  quint64 fee = CurrencyAdapter::instance().parseAmount(m_ui->m_feeSpin->cleanText());
  fee -= MESSAGE_AMOUNT * transfers.size();
  if (ttl == 0 && fee < MINIMAL_MESSAGE_FEE) {
    QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Incorrect fee value"), QtCriticalMsg));
    return;
  }

  if (WalletAdapter::instance().isOpen()) {
    WalletAdapter::instance().sendMessage(transfers, fee, m_ui->m_mixinSlider->value(), messages, ttl, messageSender);
  }
}

void SendMessageFrame::ttlCheckStateChanged(int _state) {
  recalculateFeeValue();
}

void SendMessageFrame::ttlValueChanged(int _ttlValue) {
  quint32 value = _ttlValue * MIN_TTL;
  quint32 hours = value / HOUR_SECONDS;
  quint32 minutes = value % HOUR_SECONDS / MINUTE_SECONDS;
  m_ui->m_ttlLabel->setText(QString("%1h %2m").arg(hours).arg(minutes));
}

}
