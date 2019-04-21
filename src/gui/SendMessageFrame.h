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

#pragma once

#include <QFrame>

#include <IWalletLegacy.h>

namespace Ui {
class SendMessageFrame;
}

namespace WalletGui {

class MessageAddressFrame;

class SendMessageFrame : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(SendMessageFrame)

public:
  SendMessageFrame(QWidget* _parent);
  ~SendMessageFrame();

  void setAddress(const QString& _address);

private:
  QScopedPointer<Ui::SendMessageFrame> m_ui;
  QList<MessageAddressFrame*> m_addressFrames;
  void sendMessageCompleted(CryptoNote::TransactionId _transactionId, bool _error, const QString& _errorText);
  void reset();

  QString extractAddress(const QString& _addressString) const;
  void recalculateFeeValue();

  Q_SLOT void addRecipientClicked();
  Q_SLOT void messageTextChanged();
  Q_SLOT void mixinValueChanged(int _value);
  Q_SLOT void sendClicked();
  Q_SLOT void ttlCheckStateChanged(int _state);
  Q_SLOT void ttlValueChanged(int _ttlValue);
};

}
