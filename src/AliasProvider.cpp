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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QStringList>
#include <QUrl>

#include "AliasProvider.h"

namespace WalletGui {

Q_DECL_CONSTEXPR char ALIAS_OBJECT_NAME[] = "qwertycoin";
Q_DECL_CONSTEXPR char ALIAS_NAME_TAG[] = "name";
Q_DECL_CONSTEXPR char ALIAS_ADDRESS_TAG[] = "qwc";

AliasProvider::AliasProvider(QObject *parent) : QObject(parent), m_networkManager() {
}

AliasProvider::~AliasProvider() {
}

void AliasProvider::getAddresses(const QString& _urlString) {
  QUrl url = QUrl::fromUserInput(_urlString);
  if (!url.isValid()) {
    return;
  }

  QNetworkRequest request(url);
  QNetworkReply* reply = m_networkManager.get(request);
  connect(reply, &QNetworkReply::readyRead, this, &AliasProvider::readyRead);
  connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void AliasProvider::readyRead() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull()) {
    return;
  }

  QJsonArray array = doc.object().value(ALIAS_OBJECT_NAME).toArray();
  if (array.isEmpty()) {
    return;
  }

  QJsonObject obj = array.first().toObject();
  QString name = obj.value(ALIAS_NAME_TAG).toString();
  QString address = obj.value(ALIAS_ADDRESS_TAG).toString();

  if (!address.isEmpty()) {
    Q_EMIT aliasFoundSignal(name, address);
  }
}

}
