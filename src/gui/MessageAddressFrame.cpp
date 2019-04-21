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

#include <QApplication>
#include <QClipboard>

#include "MessageAddressFrame.h"
#include "AddressBookDialog.h"
#include "AliasProvider.h"
#include "MainWindow.h"
#include "ui_messageaddressframe.h"

namespace WalletGui {

namespace {
  Q_DECL_CONSTEXPR quint32 MESSAGE_ADDRESS_INPUT_INTERVAL = 1500;
}

MessageAddressFrame::MessageAddressFrame(QWidget* _parent) : QFrame(_parent),
  m_ui(new Ui::MessageAddressFrame), m_aliasProvider(new AliasProvider(this)), m_addressInputTimerId(-1) {
  m_ui->setupUi(this);

  connect(m_aliasProvider, &AliasProvider::aliasFoundSignal, this, &MessageAddressFrame::onAliasFound);
}

MessageAddressFrame::~MessageAddressFrame() {

}

QString MessageAddressFrame::getAddress() const {
  return m_ui->m_addressEdit->text();
}

void MessageAddressFrame::disableRemoveButton(bool _disable) {
  m_ui->m_removeButton->setDisabled(_disable);
}

void MessageAddressFrame::timerEvent(QTimerEvent* _event) {
  if (_event->timerId() == m_addressInputTimerId) {
    m_aliasProvider->getAddresses(m_ui->m_addressEdit->text().trimmed());
    killTimer(m_addressInputTimerId);
    m_addressInputTimerId = -1;
    return;
  }

  QFrame::timerEvent(_event);
}

void MessageAddressFrame::onAliasFound(const QString& _name, const QString& _address) {
  m_ui->m_addressEdit->setText(QString("%1 <%2>").arg(_name).arg(_address));
}

void MessageAddressFrame::addressBookClicked() {
  AddressBookDialog dlg(&MainWindow::instance());
  if(dlg.exec() == QDialog::Accepted) {
    m_ui->m_addressEdit->setText(dlg.getAddress());
  }
}

void MessageAddressFrame::addressEdited(const QString& _text) {
  if (m_addressInputTimerId != -1) {
    killTimer(m_addressInputTimerId);
  }

  m_addressInputTimerId = startTimer(MESSAGE_ADDRESS_INPUT_INTERVAL);
}

void MessageAddressFrame::pasteClicked() {
  m_ui->m_addressEdit->setText(QApplication::clipboard()->text());
}

}
