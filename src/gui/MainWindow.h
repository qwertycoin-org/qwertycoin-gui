// Copyright (c) 2011-2016 The Cryptonote developers
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

#pragma once

#include <QLabel>
#include <QPushButton>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>
#include "ChangeLanguageDialog.h"
#include "CommandLineParser.h"
#include "PaymentServer.h"

class QActionGroup;

namespace Ui {
class MainWindow;
}

namespace WalletGui {

class MainWindow : public QMainWindow {
  Q_OBJECT
  Q_DISABLE_COPY(MainWindow)

public:
  static MainWindow& instance();
  void scrollToTransaction(const QModelIndex& _index);
  void handlePaymentRequest(QString _request);
  void isTrackingMode();
  void checkTrackingMode();
  void quit();

protected:
  void closeEvent(QCloseEvent* _event) Q_DECL_OVERRIDE;
  bool event(QEvent* _event) Q_DECL_OVERRIDE;

private:
  PaymentServer* paymentServer;

  QScopedPointer<Ui::MainWindow> m_ui;
  QPushButton* m_connectionStateIconLabel;
  QLabel* m_encryptionStateIconLabel;
  QLabel* m_synchronizationStateIconLabel;
  QLabel* m_trackingModeIconLabel;
  QLabel* m_remoteModeIconLabel;
  QSystemTrayIcon* m_trayIcon;
  QActionGroup* m_tabActionGroup;
  QAction* accountWidget;
  QAction* toggleHideAction;
  bool m_isAboutToQuit;
  QList<QAction*> recentFileActionList;
  const int maxRecentFiles;

  static MainWindow* m_instance;

  QMenu *trayIconMenu;

  MainWindow();
  ~MainWindow();

  void connectToSignals();
  void initUi();
  void createTrayIcon();
  void createTrayIconMenu();

  void minimizeToTray(bool _on);
  void setStatusBarText(const QString& _text);
  void showMessage(const QString& _text, QtMsgType _type);
  void askForWalletPassword(bool _error);
  void encryptedFlagChanged(bool _encrypted);
  void peerCountUpdated(quint64 _peer_count);
  void walletSynchronizationInProgress();
  void walletSynchronized(int _error, const QString& _error_text);
  void walletOpened(bool _error, const QString& _error_text);
  void walletClosed();
  void updateWalletAddress(const QString& _address);
  void reset();
  void onShowQR();
  void onUriOpenSignal();
  void adjustForCurrentFile(const QString& filePath);
  void updateRecentActionList();
  void updateUnmixableBalance(quint64 _balance);
  void payTo(const QModelIndex& _index);

  Q_SLOT void createWallet();
  Q_SLOT void createNonDeterministicWallet();
  Q_SLOT void openWallet();
  Q_SLOT void closeWallet();
  Q_SLOT void importKey();
  Q_SLOT void backupWallet();
  Q_SLOT void resetWallet();
  Q_SLOT void encryptWallet();
  Q_SLOT void aboutQt();
  Q_SLOT void about();
  Q_SLOT void setStartOnLogin(bool _on);
  Q_SLOT void setMinimizeToTray(bool _on);
  Q_SLOT void setMiningOnLaunch(bool _on);
  Q_SLOT void setCloseToTray(bool _on);
  Q_SLOT void ChangeLanguage();
  Q_SLOT void showPrivateKeys();
  Q_SLOT void DisplayCmdLineHelp();
  Q_SLOT void openConnectionSettings();
  Q_SLOT void exportTrackingKey();
  Q_SLOT void importTrackingKey();
  Q_SLOT void openRecent();
  Q_SLOT void showStatusInfo();
  Q_SLOT void openLogFile();
  Q_SLOT void toggleHidden();
  Q_SLOT void showNormalIfMinimized(bool fToggleHidden = false);
  Q_SLOT void showMnemonicSeed();
  Q_SLOT void restoreFromMnemonicSeed();
  Q_SLOT void sweepUnmixable();

  bool isObscured(QWidget *w);
  bool checkPoint(const QPoint &p, const QWidget *w);

#ifdef Q_OS_MAC
public:
  void restoreFromDock();

private:
  void installDockHandler();
#endif
#ifdef Q_OS_WIN
protected:
  void changeEvent(QEvent* _event) Q_DECL_OVERRIDE;

private:
  Q_SLOT void trayActivated(QSystemTrayIcon::ActivationReason _reason);
#endif
};

}
