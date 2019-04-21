// Copyright (c) 2011-2015 The Cryptonote developers
// Copyright (c) 2015-2016 XDN developers
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

#include <QRegExpValidator>
#include <QInputDialog>
#include <QMessageBox>
#include <QUrlQuery>
#include <QTime>
#include <QUrl>

#include "AddressBookModel.h"
#include "CurrencyAdapter.h"
#include "MainWindow.h"
#include "NodeAdapter.h"
#include "SendFrame.h"
#include "TransferFrame.h"
#include "WalletAdapter.h"
#include "WalletEvents.h"
#include "Settings.h"
#include "AddressProvider.h"
#include "OpenUriDialog.h"
#include "ConfirmSendDialog.h"

#include "ui_sendframe.h"

namespace WalletGui {

SendFrame::SendFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::SendFrame), m_addressProvider(new AddressProvider(this)), m_glassFrame(new SendGlassFrame(nullptr)) {
  m_ui->setupUi(this);
  m_glassFrame->setObjectName("m_sendGlassFrame");
  clearAllClicked();
  m_ui->m_mixinSlider->setValue(2);
  mixinValueChanged(m_ui->m_mixinSlider->value());
  m_ui->m_prioritySlider->setValue(2);
  priorityValueChanged(m_ui->m_prioritySlider->value());
  remote_node_fee = 0;
  amountValueChange();

  connect(&WalletAdapter::instance(), &WalletAdapter::walletSendTransactionCompletedSignal, this, &SendFrame::sendTransactionCompleted,
    Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualBalanceUpdatedSignal, this, &SendFrame::walletActualBalanceUpdated,
    Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &SendFrame::reset);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationCompletedSignal, this, &SendFrame::walletSynchronized
    , Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationProgressUpdatedSignal,
    this, &SendFrame::walletSynchronizationInProgress, Qt::QueuedConnection);

  m_ui->m_tickerLabel->setText(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_feeSpin->setSuffix(" " + CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_donateSpin->setSuffix(" " + CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  m_ui->m_remote_label->hide();
  m_ui->m_sendButton->setEnabled(false);
  m_ui->m_feeSpin->setMinimum(getMinimalFee());

  QLabel *label1 = new QLabel(tr("Low"), this);
  QLabel *label2 = new QLabel(tr("Normal"), this);
  QLabel *label3 = new QLabel(tr("High"), this);
  QLabel *label4 = new QLabel(tr("Highest"), this);
  label1->setStyleSheet(".QLabel { margin: 0; padding: 0;}");
  label2->setStyleSheet(".QLabel { margin: 0; padding: 0;}");
  label3->setStyleSheet(".QLabel { margin: 0; padding: 0;}");
  label4->setStyleSheet(".QLabel { margin: 0; padding: 0;}");
  m_ui->m_priorityGridLayout->addWidget(m_ui->m_prioritySlider, 0, 0, 1, 4);
  m_ui->m_priorityGridLayout->addWidget(label1, 1, 0, 1, 1, Qt::AlignHCenter);
  m_ui->m_priorityGridLayout->addWidget(label2, 1, 1, 1, 1, Qt::AlignHCenter);
  m_ui->m_priorityGridLayout->addWidget(label3, 1, 2, 1, 1, Qt::AlignHCenter);
  m_ui->m_priorityGridLayout->addWidget(label4, 1, 3, 1, 1, Qt::AlignHCenter);
  m_ui->m_prioritySlider->setStyleSheet(".QSlider { margin: 0 10px; padding: 0;}");
  m_ui->m_mixinSlider->setStyleSheet(".QSlider { margin: 0 10px; padding: 0;}");

  QRegExp hexMatcher("^[0-9A-F]{64}$", Qt::CaseInsensitive);
  QValidator *validator = new QRegExpValidator(hexMatcher, this);
  m_ui->m_paymentIdEdit->setValidator(validator);

  QString connection = Settings::instance().getConnection();
  if(connection.compare("remote") == 0) {
    QString remoteNodeUrl = Settings::instance().getCurrentRemoteNode() + "/feeaddress";
    m_addressProvider->getAddress(remoteNodeUrl);
    connect(m_addressProvider, &AddressProvider::addressFoundSignal, this, &SendFrame::onAddressFound, Qt::QueuedConnection);
  }
  m_ui->m_advancedWidget->hide();
}

SendFrame::~SendFrame() {
    m_transfers.clear();
    m_glassFrame->deleteLater();
}

void SendFrame::walletSynchronized(int _error, const QString& _error_text) {
  m_ui->m_sendButton->setEnabled(true);
  m_glassFrame->remove();
}

void SendFrame::walletSynchronizationInProgress(quint64 _current, quint64 _total) {
  m_glassFrame->install(this);
  m_glassFrame->updateSynchronizationState(_current, _total);
}

void SendFrame::setAddress(const QString& _address) {
  Q_FOREACH (TransferFrame* transfer, m_transfers) {
    if (transfer->getAddress().isEmpty()) {
      transfer->setAddress(_address);
      return;
    }
  }

  addRecipientClicked();
  m_transfers.last()->setAddress(_address);
}

void SendFrame::addRecipientClicked() {
  TransferFrame* newTransfer = new TransferFrame(m_ui->m_transfersScrollarea);
  m_ui->m_send_frame_layout->insertWidget(m_transfers.size(), newTransfer);
  m_transfers.append(newTransfer);
  if (m_transfers.size() == 1) {
    newTransfer->disableRemoveButton(true);
    m_ui->m_sendAllButton->setEnabled(true);
  } else {
    m_transfers[0]->disableRemoveButton(false);
    m_ui->m_sendAllButton->setEnabled(false);
  }

  connect(newTransfer, &TransferFrame::destroyed, [this](QObject* _obj) {
      m_transfers.removeOne(static_cast<TransferFrame*>(_obj));
      if (m_transfers.size() == 1) {
        m_transfers[0]->disableRemoveButton(true);
        m_ui->m_sendAllButton->setEnabled(true);
      }
    });

  connect(newTransfer, &TransferFrame::amountValueChangedSignal, this, &SendFrame::amountValueChange, Qt::QueuedConnection);
  connect(newTransfer, &TransferFrame::insertPaymentIDSignal, this, &SendFrame::insertPaymentID, Qt::QueuedConnection);
}

double SendFrame::getMinimalFee() {
  double fee(0);
  if (NodeAdapter::instance().getCurrentBlockMajorVersion() < CryptoNote::BLOCK_MAJOR_VERSION_6) {
    fee = CurrencyAdapter::instance().formatAmount(CurrencyAdapter::instance().getMinimumFee()).toDouble();
  } else {
    fee = CurrencyAdapter::instance().formatAmount(NodeAdapter::instance().getMinimalFee()).toDouble();
  }
  int digits = 2; // round up fee to 2 digits after leading zeroes
  double scale = pow(10., floor(log10(fabs(fee))) + (1 - digits));
  double roundedFee = ceil(fee / scale) * scale;
  return roundedFee;
}

void SendFrame::clearAllClicked() {
  Q_FOREACH (TransferFrame* transfer, m_transfers) {
    transfer->close();
  }
  m_transfers.clear();
  addRecipientClicked();
  amountValueChange();
  m_ui->m_paymentIdEdit->clear();
  m_ui->m_mixinSlider->setValue(7);
  m_ui->m_prioritySlider->setValue(2);
  priorityValueChanged(m_ui->m_prioritySlider->value());
}

void SendFrame::reset() {
  amountValueChange();
}

void SendFrame::amountValueChange() {
    QVector<quint64> amounts;
    Q_FOREACH (TransferFrame * transfer, m_transfers) {
      quint64 amount = CurrencyAdapter::instance().parseAmount(transfer->getAmountString());
      amounts.push_back(amount);
      }
    total_amount = 0;
    for(QVector<quint64>::iterator it = amounts.begin(); it != amounts.end(); ++it) {
      total_amount += *it;
    }

    QVector<quint64> fees;
    fees.clear();
    Q_FOREACH (TransferFrame * transfer, m_transfers) {
      quint64 amount = CurrencyAdapter::instance().parseAmount(transfer->getAmountString());
      quint64 percentfee = amount * 0.25 / 100; // fee is 0.25%
      fees.push_back(percentfee);
      }
    remote_node_fee = 0;
    if( !remote_node_fee_address.isEmpty() ) {
        for(QVector<quint64>::iterator it = fees.begin(); it != fees.end(); ++it) {
            remote_node_fee += *it;
        }
        if (remote_node_fee < NodeAdapter::instance().getMinimalFee()) {
            remote_node_fee = NodeAdapter::instance().getMinimalFee();
        }
        if (remote_node_fee > 10000000000) {
            remote_node_fee = 10000000000;
        }
    }

    QVector<float> donations;
    donations.clear();
    Q_FOREACH (TransferFrame * transfer, m_transfers) {
      float amount = transfer->getAmountString().toFloat();
      float donationpercent = amount * 0.1 / 100; // donation is 0.1%
      donations.push_back(donationpercent);
      }
    float donation_amount = 0;
    for(QVector<float>::iterator it = donations.begin(); it != donations.end(); ++it) {
        donation_amount += *it;
    }
    float min = getMinimalFee();
    if (donation_amount < min)
        donation_amount = min;
    donation_amount = floor(donation_amount * pow(10., 4) + .5) / pow(10., 4);
    m_ui->m_donateSpin->setValue(QString::number(donation_amount).toDouble());

    if( !remote_node_fee_address.isEmpty() ) {
        quint64 actualBalance = WalletAdapter::instance().getActualBalance();
        dust_balance = WalletAdapter::instance().getUnmixableBalance();
        if(actualBalance > remote_node_fee) {
            m_ui->m_balanceLabel->setText(CurrencyAdapter::instance().formatAmount(actualBalance - remote_node_fee - dust_balance));
        } else {
            m_ui->m_balanceLabel->setText(CurrencyAdapter::instance().formatAmount(actualBalance - dust_balance));
        }
        m_ui->m_remote_label->setText(QString(tr("Node fee: %1 %2")).arg(CurrencyAdapter::instance().formatAmount(remote_node_fee)).arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper()));
    }
}

void SendFrame::insertPaymentID(QString _paymentid) {
    m_ui->m_paymentIdEdit->setText(_paymentid);
}

void SendFrame::onAddressFound(const QString& _address) {
    SendFrame::remote_node_fee_address = _address;
    m_ui->m_remote_label->show();
    amountValueChange();
}

void SendFrame::openUriClicked() {
    OpenUriDialog dlg(&MainWindow::instance());
    if (dlg.exec() == QDialog::Accepted) {
        QString uri = dlg.getURI();
        if (uri.isEmpty()) {
          return;
        }
        SendFrame::parsePaymentRequest(uri);
        Q_EMIT uriOpenSignal();
    }
}

void SendFrame::parsePaymentRequest(QString _request) {
    MainWindow::instance().showNormal();
    if(_request.startsWith("qwertycoin://", Qt::CaseInsensitive))
    {
       _request.replace(0, 13, "qwertycoin:");
    }
    if(!_request.startsWith("qwertycoin:", Qt::CaseInsensitive)) {
      QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Payment request should start with qwertycoin:"), QtCriticalMsg));
      return;
    }

    if(_request.startsWith("qwertycoin:", Qt::CaseInsensitive))
    {
      _request.remove(0, 11);
    }

    QString address = _request.split("?").at(0);

    if (!CurrencyAdapter::instance().validateAddress(address)) {
      QCoreApplication::postEvent(
        &MainWindow::instance(),
        new ShowMessageEvent(tr("Invalid recipient address"), QtCriticalMsg));
      return;
    }
    m_transfers.at(0)->TransferFrame::setAddress(address);

    _request.replace("?", "&");

    QUrlQuery uriQuery(_request);

    quint64 amount = CurrencyAdapter::instance().parseAmount(uriQuery.queryItemValue("amount"));
    if(amount != 0){
        m_transfers.at(0)->TransferFrame::setAmount(amount);
    }

    QString label = uriQuery.queryItemValue("label");
    if(!label.isEmpty()){
        m_transfers.at(0)->TransferFrame::setLabel(label);
    }

    QString payment_id = uriQuery.queryItemValue("payment_id");
    if(!payment_id.isEmpty()){
        SendFrame::insertPaymentID(payment_id);
    }
}

void SendFrame::sendClicked() {
 ConfirmSendDialog dlg(&MainWindow::instance());
    dlg.showPaymentDetails(total_amount);
    if (m_ui->donateCheckBox->isChecked()) {
      dlg.showConfirmDonation(total_amount*1/1000);
    }
    if(!m_ui->m_paymentIdEdit->text().isEmpty()){
      dlg.showPaymentId(m_ui->m_paymentIdEdit->text());
    } else {
      dlg.confirmNoPaymentId();
    }
    if (dlg.exec() == QDialog::Accepted) {

      std::vector<CryptoNote::WalletLegacyTransfer> walletTransfers;
      Q_FOREACH (TransferFrame * transfer, m_transfers) {
        QString address = transfer->getAddress();
        if (!CurrencyAdapter::instance().validateAddress(address)) {
          QCoreApplication::postEvent(
            &MainWindow::instance(),
            new ShowMessageEvent(tr("Invalid recipient address"), QtCriticalMsg));
          return;
        }

        CryptoNote::WalletLegacyTransfer walletTransfer;
        walletTransfer.address = address.toStdString();
        uint64_t amount = CurrencyAdapter::instance().parseAmount(transfer->getAmountString());
        walletTransfer.amount = amount;
        walletTransfers.push_back(walletTransfer);
        QString label = transfer->getLabel();
        if (!label.isEmpty()) {
          AddressBookModel::instance().addAddress(label, address, m_ui->m_paymentIdEdit->text().toUtf8());
        }
      }

      // Dev donation
      if (m_ui->donateCheckBox->isChecked()) {
          CryptoNote::WalletLegacyTransfer walletTransfer;
          walletTransfer.address = "QWC1RALGaP5U8BLJskYR2YVSjr3DQEEuS5xghbtX2mm134YVXgS4RJHZGkeBvXf4BRFLWkv4zHGJ267S9pjwvVt63xwkdYPCwF";
          walletTransfer.amount = CurrencyAdapter::instance().parseAmount(m_ui->m_donateSpin->cleanText());
          walletTransfers.push_back(walletTransfer);
      }

      // Remote node fee
      QString connection = Settings::instance().getConnection();
      if(connection.compare("remote") == 0) {
          if (!SendFrame::remote_node_fee_address.isEmpty()) {
            CryptoNote::WalletLegacyTransfer walletTransfer;
            walletTransfer.address = SendFrame::remote_node_fee_address.toStdString();
            walletTransfer.amount = remote_node_fee;
            walletTransfers.push_back(walletTransfer);
          }
      }

      // Miners fee
      priorityValueChanged(m_ui->m_prioritySlider->value());
      quint64 fee = CurrencyAdapter::instance().parseAmount(m_ui->m_feeSpin->cleanText());

	  if (fee < NodeAdapter::instance().getMinimalFee()) {
        QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Incorrect fee value"), QtCriticalMsg));
        return;
      }

      quint64 total_transaction_amount = 0;
      for (size_t i = 0; i < walletTransfers.size(); i++){
        total_transaction_amount += walletTransfers.at(i).amount;
      }
      if (total_transaction_amount > (WalletAdapter::instance().getActualBalance() - fee - dust_balance)) {
        QMessageBox::critical(this, tr("Insufficient balance"), tr("Available balance is insufficient to send this transaction. Have you excluded a fee?"), QMessageBox::Ok);
        return;
      }

      if (WalletAdapter::instance().isOpen()) {
          QByteArray paymentIdString = m_ui->m_paymentIdEdit->text().toUtf8();
          if (!isValidPaymentId(paymentIdString)) {
            QCoreApplication::postEvent(&MainWindow::instance(), new ShowMessageEvent(tr("Invalid payment ID"), QtCriticalMsg));
            return;
          }

          WalletAdapter::instance().sendTransaction(walletTransfers, fee, m_ui->m_paymentIdEdit->text(), m_ui->m_mixinSlider->value());
      }
  }
}

void SendFrame::mixinValueChanged(int _value) {
  m_ui->m_mixinLabel->setText(QString::number(_value));
}

void SendFrame::priorityValueChanged(int _value) {
  double send_fee = getMinimalFee() * _value;
  m_ui->m_feeSpin->setValue(send_fee);
}

void SendFrame::sendTransactionCompleted(CryptoNote::TransactionId _id, bool _error, const QString& _error_text) {
  Q_UNUSED(_id);
  if (_error) {
    QCoreApplication::postEvent(
      &MainWindow::instance(),
      new ShowMessageEvent(_error_text, QtCriticalMsg));
  } else {
    clearAllClicked();
  }
}

void SendFrame::walletActualBalanceUpdated(quint64 _balance) {
  dust_balance = WalletAdapter::instance().getUnmixableBalance();
  if(!remote_node_fee_address.isEmpty() && _balance > remote_node_fee) {
    m_ui->m_balanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance - remote_node_fee - dust_balance));
  } else {
    m_ui->m_balanceLabel->setText(CurrencyAdapter::instance().formatAmount(_balance - dust_balance));
  }
}

bool SendFrame::isValidPaymentId(const QByteArray& _paymentIdString) {
  if (_paymentIdString.isEmpty()) {
    return true;
  }

  QByteArray paymentId = QByteArray::fromHex(_paymentIdString);
  return (paymentId.size() == sizeof(Crypto::Hash)) && (_paymentIdString.toUpper() == paymentId.toHex().toUpper());
}

void SendFrame::generatePaymentIdClicked() {
  SendFrame::insertPaymentID(CurrencyAdapter::instance().generatePaymentId());
}

void SendFrame::advancedClicked(bool _show) {
  if (_show) {
    m_ui->m_advancedWidget->show();
  } else {
    m_ui->m_advancedWidget->hide();
  }
}

void SendFrame::sendAllClicked() {
  quint64 actualBalance = WalletAdapter::instance().getActualBalance();
  if (actualBalance == 0)
      return;
  dust_balance = WalletAdapter::instance().getUnmixableBalance();
  if (dust_balance != 0) {
    QCoreApplication::postEvent(
      &MainWindow::instance(),
      new ShowMessageEvent(tr("You have unmixable dust on balance. Use menu 'Wallet -> Sweep unmixable' first."), QtCriticalMsg));
    return;
  }
  remote_node_fee = 0;
  if(!remote_node_fee_address.isEmpty()) {
    remote_node_fee = static_cast<qint64>(actualBalance * 0.0025); // fee is 0.25%
    if (remote_node_fee < NodeAdapter::instance().getMinimalFee()) {
        remote_node_fee = NodeAdapter::instance().getMinimalFee();
    }
    if (remote_node_fee > 10000000000) {
        remote_node_fee = 10000000000;
    }
  }

  QVector<float> donations;
  donations.clear();
  Q_FOREACH (TransferFrame * transfer, m_transfers) {
    float amount = transfer->getAmountString().toFloat();
    float donationpercent = amount * 0.1 / 100; // donation is 0.1%
    donations.push_back(donationpercent);
    }
  quint64 priorityFee = CurrencyAdapter::instance().parseAmount(QString::number(getMinimalFee() * m_ui->m_prioritySlider->value()));
  quint64 amount = actualBalance - (priorityFee + remote_node_fee);
  if (m_ui->donateCheckBox->isChecked()) {
    float donation_amount = CurrencyAdapter::instance().formatAmount(amount).toDouble() * 0.1 / 100;
    donation_amount = floor(donation_amount * pow(10., 4) + .5) / pow(10., 4);
    float min = getMinimalFee();
    if (donation_amount < min)
        donation_amount = min;
    amount -= static_cast<quint64>(CurrencyAdapter::instance().parseAmount(QString::number(donation_amount)));
    m_transfers[0]->setAmount(amount);
    m_ui->m_donateSpin->setValue(QString::number(donation_amount).toDouble());
  } else {
    m_transfers[0]->setAmount(amount);
  }
}

}
