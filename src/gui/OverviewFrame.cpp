// Copyright (c) 2011-2015 The Cryptonote developers
// Copyright (c) 2017-2018, The karbo developers
// Copyright (c) 2018, The Qwertcoin developers
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
// along with Qwertycoin. If not, see <http://www.gnu.org/licenses/>.

#include "CurrencyAdapter.h"
#include "OverviewFrame.h"
#include "TransactionFrame.h"
#include "RecentTransactionsModel.h"
#include "WalletAdapter.h"

#include "ui_overviewframe.h"

namespace WalletGui {

class RecentTransactionsDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  RecentTransactionsDelegate(QObject* _parent) : QStyledItemDelegate(_parent) {
  }

  ~RecentTransactionsDelegate() {
  }

  QWidget* createEditor(QWidget* _parent, const QStyleOptionViewItem& _option, const QModelIndex& _index) const Q_DECL_OVERRIDE {
    if (!_index.isValid()) {
      return nullptr;
    }

    return new TransactionFrame(_index, _parent);
  }

  QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const Q_DECL_OVERRIDE {
    return QSize(346, 64);
  }
};

OverviewFrame::OverviewFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::OverviewFrame), m_transactionModel(new RecentTransactionsModel) {
  m_ui->setupUi(this);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualBalanceUpdatedSignal, this, &OverviewFrame::updateActualBalance,
    Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingBalanceUpdatedSignal, this, &OverviewFrame::updatePendingBalance,
    Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletUnmixableBalanceUpdatedSignal, this, &OverviewFrame::updateUnmixableBalance,
    Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &OverviewFrame::reset,
    Qt::QueuedConnection);
  connect(m_transactionModel.data(), &QAbstractItemModel::rowsInserted, this, &OverviewFrame::transactionsInserted);
  connect(m_transactionModel.data(), &QAbstractItemModel::layoutChanged, this, &OverviewFrame::layoutChanged);

  m_ui->m_tickerLabel1->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel2->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel3->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_tickerLabel4->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());

  m_ui->m_recentTransactionsView->setItemDelegate(new RecentTransactionsDelegate(this));
  m_ui->m_recentTransactionsView->setModel(m_transactionModel.data());
  reset();
}

OverviewFrame::~OverviewFrame() {
}

void OverviewFrame::transactionsInserted(const QModelIndex& _parent, int _first, int _last) {
  for (quint32 i = _first; i <= _last; ++i) {
    QModelIndex recentModelIndex = m_transactionModel->index(i, 0);
    m_ui->m_recentTransactionsView->openPersistentEditor(recentModelIndex);
  }
}

void OverviewFrame::layoutChanged() {
  for (quint32 i = 0; i <= m_transactionModel->rowCount(); ++i) {
    QModelIndex recent_index = m_transactionModel->index(i, 0);
    m_ui->m_recentTransactionsView->openPersistentEditor(recent_index);
  }
}

void OverviewFrame::updateActualBalance(quint64 _balance) {
  m_ui->m_actualBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();
  m_ui->m_totalBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + pendingBalance));
}

void OverviewFrame::updatePendingBalance(quint64 _balance) {
  m_ui->m_pendingBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance));
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  m_ui->m_totalBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance + actualBalance));
}

void OverviewFrame::updateUnmixableBalance(quint64 _balance) {
  m_ui->m_unmixableBalanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance).remove(','));
}

void OverviewFrame::reset() {
  updateActualBalance(0);
  updatePendingBalance(0);
  updateUnmixableBalance(0);
}

}

#include "OverviewFrame.moc"
