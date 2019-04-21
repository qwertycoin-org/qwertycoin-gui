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

#include "MessagesFrame.h"
#include "MainWindow.h"
#include "MessageDetailsDialog.h"
#include "MessagesModel.h"
#include "SortedMessagesModel.h"
#include "VisibleMessagesModel.h"

#include "ui_messagesframe.h"

namespace WalletGui {

MessagesFrame::MessagesFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::MessagesFrame),
  m_visibleMessagesModel(new VisibleMessagesModel) {
  m_ui->setupUi(this);
  m_ui->m_messagesView->setModel(m_visibleMessagesModel.data());
  m_ui->m_messagesView->header()->resizeSection(MessagesModel::COLUMN_DATE, 140);
  m_ui->m_replyButton->setEnabled(false);

  connect(m_ui->m_messagesView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MessagesFrame::currentMessageChanged);
}

MessagesFrame::~MessagesFrame() {
}

void MessagesFrame::currentMessageChanged(const QModelIndex& _currentIndex) {
  m_ui->m_replyButton->setEnabled(_currentIndex.isValid() && !_currentIndex.data(MessagesModel::ROLE_HEADER_REPLY_TO).toString().isEmpty());
}

void MessagesFrame::messageDoubleClicked(const QModelIndex& _index) {
  if (!_index.isValid()) {
    return;
  }

  MessageDetailsDialog dlg(_index, &MainWindow::instance());
  if (dlg.exec() == QDialog::Accepted) {
    Q_EMIT replyToSignal(dlg.getCurrentMessageIndex());
  }
}

void MessagesFrame::replyClicked() {
  Q_EMIT replyToSignal(m_ui->m_messagesView->selectionModel()->currentIndex());
}

}
