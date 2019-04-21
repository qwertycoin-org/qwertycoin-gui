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

#include <QDateTime>
#include <QList>
#include <QPair>

namespace WalletGui {

typedef QList<QPair<QString, QString> > MessageHeader;

class Message {
public:
  Message();
  Message(const Message& _message);
  Message(const Message&& _message);
  Message(const QString& _message);
  ~Message();

  QString getMessage() const;
  QString getFullMessage() const;
  QString getHeaderValue(const QString& _key) const;

  Message& operator=(const Message& _message);

  static QString makeTextMessage(const QString& _message, const MessageHeader& _header);

private:
  QString m_message;
  MessageHeader m_header;
};

}
