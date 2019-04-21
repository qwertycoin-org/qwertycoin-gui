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

#include "VisibleMessagesModel.h"
#include "MessagesModel.h"
#include "SortedMessagesModel.h"

namespace WalletGui {

VisibleMessagesModel::VisibleMessagesModel() : QSortFilterProxyModel() {
  setSourceModel(&SortedMessagesModel::instance());
}

VisibleMessagesModel::~VisibleMessagesModel() {
}

bool VisibleMessagesModel::filterAcceptsColumn(int _sourceColumn, const QModelIndex& _sourceParent) const {
  return _sourceColumn == MessagesModel::COLUMN_DATE || _sourceColumn == MessagesModel::COLUMN_TYPE ||
    _sourceColumn == MessagesModel::COLUMN_HEIGHT || _sourceColumn == MessagesModel::COLUMN_MESSAGE;
}

}
