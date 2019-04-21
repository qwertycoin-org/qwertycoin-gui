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

#include <QFileDialog>
#include <QMessageBox>

#include "MessageDetailsDialog.h"
#include "MainWindow.h"
#include "MessagesModel.h"

#include "ui_messagedetailsdialog.h"

namespace WalletGui {

MessageDetailsDialog::MessageDetailsDialog(const QModelIndex& _index, QWidget* _parent) : QDialog(_parent),
  m_ui(new Ui::MessageDetailsDialog) {
  m_ui->setupUi(this);
  QModelIndex modelIndex = MessagesModel::instance().index(_index.data(MessagesModel::ROLE_ROW).toInt(), 0);
  m_dataMapper.setModel(&MessagesModel::instance());
  m_dataMapper.addMapping(m_ui->m_heightLabel, MessagesModel::COLUMN_HEIGHT, "text");
  m_dataMapper.addMapping(m_ui->m_hashLabel, MessagesModel::COLUMN_HASH, "text");
  m_dataMapper.addMapping(m_ui->m_amountLabel, MessagesModel::COLUMN_AMOUNT, "text");
  m_dataMapper.addMapping(m_ui->m_sizeLabel, MessagesModel::COLUMN_MESSAGE_SIZE, "text");
  m_dataMapper.addMapping(m_ui->m_messageTextEdit, MessagesModel::COLUMN_FULL_MESSAGE, "plainText");
  m_dataMapper.addMapping(m_ui->m_replyButton, MessagesModel::COLUMN_HAS_REPLY_TO, "enabled");
  m_dataMapper.setCurrentModelIndex(modelIndex);

  m_ui->m_prevButton->setEnabled(m_dataMapper.currentIndex() > 0);
  m_ui->m_nextButton->setEnabled(m_dataMapper.currentIndex() < MessagesModel::instance().rowCount() - 1);
}

MessageDetailsDialog::~MessageDetailsDialog() {
}

QModelIndex MessageDetailsDialog::getCurrentMessageIndex() const {
  return MessagesModel::instance().index(m_dataMapper.currentIndex(), 0);
}

void MessageDetailsDialog::prevClicked() {
  m_dataMapper.toPrevious();
  m_ui->m_prevButton->setEnabled(m_dataMapper.currentIndex() > 0);
  m_ui->m_nextButton->setEnabled(m_dataMapper.currentIndex() < MessagesModel::instance().rowCount() - 1);
}

void MessageDetailsDialog::nextClicked() {
  m_dataMapper.toNext();
  m_ui->m_prevButton->setEnabled(m_dataMapper.currentIndex() > 0);
  m_ui->m_nextButton->setEnabled(m_dataMapper.currentIndex() < MessagesModel::instance().rowCount() - 1);
}

void MessageDetailsDialog::saveClicked() {
  QString filePath = QFileDialog::getSaveFileName(this, tr("Save message"), QDir::homePath());
  if (!filePath.isEmpty()) {
    QFile file(filePath);
    if (file.exists()) {
      if (QMessageBox::warning(&MainWindow::instance(), tr("File already exists"),
        tr("Warning! File already exists and will be overwritten, are you sure?"), QMessageBox::Cancel, QMessageBox::Ok) != QMessageBox::Ok) {
        return;
      }
    }

    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
      QMessageBox::critical(&MainWindow::instance(), tr("File error"), file.errorString());
      return;
    }

    QString message = m_ui->m_messageTextEdit->toPlainText();
    file.write(message.toUtf8());
    file.close();
  }
}

}
