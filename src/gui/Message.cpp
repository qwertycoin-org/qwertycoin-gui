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

#include <QTextStream>

#include "Message.h"

namespace WalletGui {

namespace {

bool isEmptyChar(const QChar& _char) {
  return _char == ' ' || _char == '\t' || _char == '\n' || _char == '\r';
}

MessageHeader parseMessage(QString& _message) {
  QString tmpMessage(_message);
  MessageHeader res;
  while (!tmpMessage.isEmpty() && isEmptyChar(tmpMessage[0])) {
    tmpMessage.remove(0, 1);
  }

  if(tmpMessage.isEmpty()) {
    return res;
  }

  QTextStream messageStream(&tmpMessage);
  while (!messageStream.atEnd()) {
    QString line = messageStream.readLine();
    if (line.isEmpty()) {
      _message = tmpMessage.mid(messageStream.pos());
      break;
    }

    QStringList keyValue = line.split(":");
    if (keyValue.size() < 2) {
      return MessageHeader();
    }

    res.append(qMakePair(keyValue[0].trimmed(), keyValue[1].trimmed()));
  }

  return res;
}

}

Message::Message() : m_message(), m_header() {
}

Message::Message(const Message& _message) : m_message(_message.m_message), m_header(_message.m_header) {
}

Message::Message(const Message&& _message) : m_message(std::move(_message.m_message)), m_header(std::move(_message.m_header)) {
}

Message::Message(const QString& _message) : m_message(_message), m_header(parseMessage(m_message)) {
}

Message::~Message() {
}

QString Message::getMessage() const {
  return m_message;
}

QString Message::getFullMessage() const {
  return makeTextMessage(m_message, m_header);
}

QString Message::getHeaderValue(const QString& _key) const {
  for (const auto& header : m_header) {
    if (header.first.compare(_key, Qt::CaseInsensitive) == 0) {
      return header.second;
    }
  }

  return QString();
}

Message& Message::operator=(const Message& _message) {
  m_message = _message.m_message;
  m_header = _message.m_header;
  return *this;
}

QString Message::makeTextMessage(const QString& _message, const MessageHeader& _header) {
  QString res;
  Q_FOREACH(const auto& headerItem, _header) {
    res.append(QString("%1: %2\n").arg(headerItem.first).arg(headerItem.second));
  }

  if (!res.isEmpty()) {
    res.append("\n");
  }

  res.append(_message);

  return res;
}

}
