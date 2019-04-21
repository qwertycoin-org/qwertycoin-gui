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

namespace Ui {
  class MessageAddressFrame;
}

namespace WalletGui {

class AliasProvider;

class MessageAddressFrame : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(MessageAddressFrame)

public:
  MessageAddressFrame(QWidget* _parent);
  ~MessageAddressFrame();

  QString getAddress() const;
  void disableRemoveButton(bool _disable);

protected:
  void timerEvent(QTimerEvent* _event) Q_DECL_OVERRIDE;

private:
  QScopedPointer<Ui::MessageAddressFrame> m_ui;
  AliasProvider* m_aliasProvider;
  int m_addressInputTimerId;

  void onAliasFound(const QString& _name, const QString& _address);

  Q_SLOT void addressBookClicked();
  Q_SLOT void addressEdited(const QString& _text);
  Q_SLOT void pasteClicked();
};

}
