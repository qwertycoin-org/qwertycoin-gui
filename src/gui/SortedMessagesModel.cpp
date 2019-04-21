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

#include <QDateTime>

#include "SortedMessagesModel.h"
#include "MessagesModel.h"

namespace WalletGui {

SortedMessagesModel& SortedMessagesModel::instance() {
  static SortedMessagesModel inst;
  return inst;
}

SortedMessagesModel::SortedMessagesModel() : QSortFilterProxyModel() {
  setSourceModel(&MessagesModel::instance());
  setDynamicSortFilter(true);
  sort(MessagesModel::COLUMN_DATE, Qt::DescendingOrder);
}

SortedMessagesModel::~SortedMessagesModel() {
}

bool SortedMessagesModel::lessThan(const QModelIndex& _left, const QModelIndex& _right) const {
  QDateTime leftDate = _left.data(MessagesModel::ROLE_DATE).toDateTime();
  QDateTime rightDate = _right.data(MessagesModel::ROLE_DATE).toDateTime();
  if((rightDate.isNull() || !rightDate.isValid()) && (leftDate.isNull() || !leftDate.isValid())) {
    return _left.row() < _right.row();
  }

  if(leftDate.isNull() || !leftDate.isValid()) {
    return false;
  }

  if(rightDate.isNull() || !rightDate.isValid()) {
    return true;
  }

  return leftDate < rightDate;
}

}
