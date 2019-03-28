set(TARGET_NAME "${PROJECT_DISPLAY_NAME}Executable")
set(TARGET_NAME_ALIAS "${PROJECT_DISPLAY_NAME}::Executable")

set(TARGET_FORMS
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/aboutdialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/accountframe.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/addressbookdialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/addressbookframe.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/changelanguagedialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/changepassworddialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/confirmsenddialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/connectionsettingsdialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/exitwidget.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/exporttrackingkeydialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/getbalanceproofdialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/importkeydialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/importtrackingkeydialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/infodialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/mainwindow.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/mnemonicseeddialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/newaddressdialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/newnodedialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/newpassworddialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/nowalletframe.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/openuridialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/optimizationsettingsdialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/overviewframe.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/passworddialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/privatekeysdialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/receiveframe.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/restorefrommnemonicseeddialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/sendframe.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/showpaymentrequest.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/signmessagedialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/transactiondetailsdialog.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/transactionframe.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/transactionsframe.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/transferframe.ui"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ui/verifymnemonicseeddialog.ui"
)

qt5_wrap_ui(TARGET_UI ${TARGET_FORMS})

set(TARGET_SRC
    "${CMAKE_CURRENT_LIST_DIR}/AddressProvider.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/AddressProvider.h"
    "${CMAKE_CURRENT_LIST_DIR}/CommandLineParser.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandLineParser.h"
    "${CMAKE_CURRENT_LIST_DIR}/CryptoNoteWrapper.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CryptoNoteWrapper.h"
    "${CMAKE_CURRENT_LIST_DIR}/CurrencyAdapter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CurrencyAdapter.h"
    "${CMAKE_CURRENT_LIST_DIR}/DnsLookup.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/DnsLookup.h"
    "${CMAKE_CURRENT_LIST_DIR}/Languages.pro"
    "${CMAKE_CURRENT_LIST_DIR}/LoggerAdapter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/LoggerAdapter.h"
    "${CMAKE_CURRENT_LIST_DIR}/NodeAdapter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/NodeAdapter.h"
    "${CMAKE_CURRENT_LIST_DIR}/OptimizationManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/OptimizationManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/PaymentServer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PaymentServer.h"
    "${CMAKE_CURRENT_LIST_DIR}/Settings.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Settings.h"
    "${CMAKE_CURRENT_LIST_DIR}/SignalHandler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SignalHandler.h"
    "${CMAKE_CURRENT_LIST_DIR}/TranslatorManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/TranslatorManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/Update.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Update.h"
    "${CMAKE_CURRENT_LIST_DIR}/WalletAdapter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/WalletAdapter.h"
    "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/miniupnpcstrings.h"
    "${CMAKE_CURRENT_LIST_DIR}/resources.qrc"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AboutDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AboutDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AccountFrame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AccountFrame.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AddressBookDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AddressBookDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AddressBookFrame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AddressBookFrame.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AddressBookModel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AddressBookModel.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AnimatedLabel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/AnimatedLabel.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ChangePasswordDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ChangePasswordDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ConfirmSendDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ConfirmSendDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ConnectionSettings.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ConnectionSettings.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ExitWidget.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ExitWidget.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ExportTrackingKeyDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ExportTrackingKeyDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/GetBalanceProofDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/GetBalanceProofDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/GlassFrame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/GlassFrame.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ImportKeyDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ImportKeyDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ImportTrackingKeyDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ImportTrackingKeyDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/InfoDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/InfoDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/MainWindow.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/MainWindow.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/MnemonicSeedDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/MnemonicSeedDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/NewAddressDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/NewAddressDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/NewNodeDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/NewNodeDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/NewPasswordDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/NewPasswordDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/NoWalletFrame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/NoWalletFrame.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/NodeModel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/NodeModel.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/OpenUriDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/OpenUriDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/OptimizationSettings.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/OptimizationSettings.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/OverviewFrame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/OverviewFrame.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/PasswordDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/PasswordDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/PrivateKeysDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/PrivateKeysDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/QRLabel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/QRLabel.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ReceiveFrame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ReceiveFrame.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/RecentSortedTransactionsModel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/RecentSortedTransactionsModel.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/RecentTransactionsModel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/RecentTransactionsModel.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/RestoreFromMnemonicSeedDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/RestoreFromMnemonicSeedDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/SendFrame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/SendFrame.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/SendGlassFrame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/SendGlassFrame.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ShowPaymentRequestDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/ShowPaymentRequestDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/SignMessageDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/SignMessageDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/SortedAddressBookModel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/SortedAddressBookModel.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/SortedTransactionsModel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/SortedTransactionsModel.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransactionDetailsDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransactionDetailsDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransactionFrame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransactionFrame.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransactionsFrame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransactionsFrame.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransactionsListModel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransactionsListModel.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransactionsModel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransactionsModel.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransferFrame.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/TransferFrame.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/VerifyMnemonicSeedDialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/gui/VerifyMnemonicSeedDialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/gui/WalletEvents.h"
    ${TARGET_UI}
)
if(APPLE)
    list(APPEND TARGET_SRC "${CMAKE_CURRENT_LIST_DIR}/gui/MainWindow.mm")
    list(APPEND TARGET_SRC "${CMAKE_CURRENT_LIST_DIR}/gui/macdockiconhandler.h")
    list(APPEND TARGET_SRC "${CMAKE_CURRENT_LIST_DIR}/gui/macdockiconhandler.mm")
elseif(WIN32)
    list(APPEND TARGET_SRC "${CMAKE_CURRENT_LIST_DIR}/cryptonotewallet.rc")
endif()

# Include directories
set(TARGET_INCLUDE_DIRS
    PRIVATE
    ${Boost_INCLUDE_DIRS}
    "${qrencode_INCLUDE_DIRS}"
    "${Qt5Core_INCLUDE_DIRS}"
    "${Qt5Gui_INCLUDE_DIRS}"
    "${Qt5Network_INCLUDE_DIRS}"
    "${Qt5Widgets_INCLUDE_DIRS}"
    "${QwertycoinFramework_INCLUDE_DIRS}"
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

# Linked libraries
set(TARGET_LINK_LIBS
    PRIVATE
    qrencode::qrencode
    Qt5::Core
    Qt5::Gui
    Qt5::Network
    Qt5::Widgets
    QwertycoinFramework::BlockchainExplorer
    QwertycoinFramework::Common
    QwertycoinFramework::Crypto
    QwertycoinFramework::CryptoNoteCore
    QwertycoinFramework::CryptoNoteProtocol
    QwertycoinFramework::Http
    QwertycoinFramework::InProcessNode
    QwertycoinFramework::JsonRpcServer
    QwertycoinFramework::Logging
    QwertycoinFramework::Mnemonics
    QwertycoinFramework::NodeRpcProxy
    QwertycoinFramework::P2p
    QwertycoinFramework::PaymentGate
    QwertycoinFramework::Rpc
    QwertycoinFramework::Serialization
    QwertycoinFramework::System
    QwertycoinFramework::Transfers
    QwertycoinFramework::Wallet
)
if(APPLE)
    find_library(Cocoa_UNIT Cocoa)
    list(APPEND TARGET_LINK_LIBS ${Cocoa_UNIT})

    list(APPEND TARGET_LINK_LIBS Qt5::PrintSupport)
elseif(WIN32)
    list(APPEND TARGET_LINK_LIBS Imm32 Iphlpapi Winmm)
endif()

# Compile definitions
set(TARGET_COMPILE_DEFS
    PRIVATE
    -DVERSION="${PROJECT_VERSION}"
)
if(WIN32)
    list(APPEND TARGET_COMPILE_DEFS -D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0600 -DSTATICLIB)
endif()

# Generate CryptoNoteWalletConfig.h header
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/CryptoNoteWalletConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CryptoNoteWalletConfig.h"
    @ONLY
)
message(STATUS "CryptoNoteWalletConfig.h: ${CMAKE_CURRENT_BINARY_DIR}/CryptoNoteWalletConfig.h")

add_executable(${TARGET_NAME} WIN32 MACOSX_BUNDLE ${TARGET_SRC})
add_executable(${TARGET_NAME_ALIAS} ALIAS ${TARGET_NAME})
target_include_directories(${TARGET_NAME} ${TARGET_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${TARGET_LINK_LIBS})
target_compile_definitions(${TARGET_NAME} ${TARGET_COMPILE_DEFS})
set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME "Qwertycoin")

if(UNIX AND NOT APPLE AND NOT ANDROID) # linux
    install(
        TARGETS ${TARGET_NAME}
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE
    )
    install(
        FILES "${CMAKE_CURRENT_LIST_DIR}/qwertycoinwallet.desktop"
        DESTINATION share/applications
    )
    install(
        FILES "${CMAKE_CURRENT_LIST_DIR}/images/qwertycoinz.png"
        DESTINATION share/pixmaps
        RENAME qwertycoin.png
    )
    install(
        DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/languages"
        DESTINATION /opt/qwertycoin/
        FILES_MATCHING PATTERN "*.qm"
    )
    install(
        FILES "${CMAKE_SOURCE_DIR}/LICENSE.txt"
        DESTINATION share/doc/qwertycoinwallet
    )
elseif(APPLE) # macos
    install(TARGETS ${TARGET_NAME} BUNDLE DESTINATION .)
    set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/images/qwertycoin.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
elseif(WIN32) # windows
    install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION .)
endif()
