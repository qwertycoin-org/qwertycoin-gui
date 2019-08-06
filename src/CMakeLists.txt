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
    list(APPEND TARGET_SRC "${CMAKE_CURRENT_LIST_DIR}/qwertycoinwallet.rc")
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

## Linked libraries
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
    list(APPEND TARGET_LINK_LIBS Imm32 Iphlpapi Winmm UxTheme d3d9 dxguid Rpcrt4 Wtsapi32 WSock32 ws2_32 Dwmapi)
endif()

# Compile definitions
set(TARGET_COMPILE_DEFS
    PRIVATE
    -DCN_VERSION="${PROJECT_VERSION}"
    -DCN_REVISION="${PROJECT_GIT_COMMIT_ID}"
    -DGIT_REVISION="${PROJECT_GIT_COMMIT_ID}"
    -DUSE_LITE_WALLET
)
if(WIN32)
    list(APPEND TARGET_COMPILE_DEFS -D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0600 -DSTATICLIB)
endif()

# Generate version.h header
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
    @ONLY
)
message(STATUS "version.h: ${CMAKE_CURRENT_BINARY_DIR}/version.h")

add_executable(${TARGET_NAME} WIN32 MACOSX_BUNDLE ${TARGET_SRC})
add_executable(${TARGET_NAME_ALIAS} ALIAS ${TARGET_NAME})
target_include_directories(${TARGET_NAME} ${TARGET_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${TARGET_LINK_LIBS})
target_compile_definitions(${TARGET_NAME} ${TARGET_COMPILE_DEFS})

if(PROJECT_OS_LINUX)
    set_target_properties(
        ${TARGET_NAME}
        PROPERTIES
            OUTPUT_NAME "qwertycoin-gui"
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "$OIGIN/../lib"
    )
else()
    set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_DISPLAY_NAME}")
endif()

if(PROJECT_OS_LINUX OR PROJECT_OS_POSIX) # Linux
    message(STATUS "Configuring Linux installation targets...")

    foreach( Qt5Module_Name Qt5Core Qt5Gui Qt5Network Qt5Widgets)
        install(
            DIRECTORY "${QT_INSTALL_LIBS}/"
            DESTINATION lib
            FILES_MATCHING
            PATTERN "${CMAKE_SHARED_LIBRARY_PREFIX}${Qt5Module_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}*"
            PATTERN "cmake*" EXCLUDE
            PATTERN "pkgconfig*" EXCLUDE
        )
    endforeach()

    install(
        FILES "${CMAKE_CURRENT_LIST_DIR}/qwertycoinwallet.sh"
        DESTINATION bin
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
        RENAME qwertycoin-gui
    )

    install(
        FILES "${CMAKE_CURRENT_LIST_DIR}/qwertycoinwallet.desktop"
        DESTINATION share/applications
    )

    install(
        FILES "${CMAKE_CURRENT_LIST_DIR}/images/qwertycoinz.png"
        DESTINATION icons/hicolor/256x256/apps
    )

    install(
        TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION lib # NOT bin!
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static
    )
elseif(PROJECT_OS_MACOS) # macOS
    message(STATUS "Configuring macOS installation targets...")

    set(MACOSX_BUNDLE_BUNDLE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_DISPLAY_NAME}.app")
    set(MACOSX_BUNDLE_ICON_FILE_NAME "qwertycoinz.icns")

    set_target_properties(
        ${TARGET_NAME}
        PROPERTIES
            OUTPUT_NAME "${PROJECT_DISPLAY_NAME}"
            MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_DISPLAY_NAME}"
            MACOSX_BUNDLE_EXECUTABLE_NAME "${PROJECT_DISPLAY_NAME}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
            MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "${PROJECT_VENDOR_DOMAIN_REVERSED}.${PROJECT_SHORT_NAME}"
            MACOSX_BUNDLE_INFO_STRING "" # Don't use. Depricated in Mac OS X 10.5 and newer.
            MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE_NAME}"
            MACOSX_BUNDLE_COPYRIGHT "(c) 2000-2099 ${PROJECT_VENDOR_NAME}"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/Info.plist.in"
    )

    set(ICON_IN "${CMAKE_CURRENT_LIST_DIR}/images/${MACOSX_BUNDLE_ICON_FILE_NAME}")
    set(ICON_OUT "$<TARGET_FILE_DIR:${TARGET_NAME}>/../Resources/${MACOSX_BUNDLE_ICON_FILE_NAME}")
    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Creating *.app/Contents/Resources directory."
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TARGET_NAME}>/../Resources
        COMMAND ${CMAKE_COMMAND} -E echo "Copying the .icns file into *.app/Contents/Resources dir."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ICON_IN} ${ICON_OUT}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Copying the .icns file into application bundle's Contents/Resources directory."
    )

    if(TARGET Qt5::macdeployqt AND EXISTS "${MACOSX_BUNDLE_BUNDLE_PATH}")
        string(
            JOIN " " MACDEPLOYQT_ARGS
            "${MACOSX_BUNDLE_BUNDLE_PATH}" -always-overwrite -appstore-compliant
        )
        install(CODE "execute_process(COMMAND $<TARGET_FILE:Qt5::macdeployqt> ${MACDEPLOYQT_ARGS})")
    endif()

    install(
        TARGETS ${TARGET_NAME}
        BUNDLE DESTINATION .
    )
elseif(PROJECT_OS_WINDOWS) # Windows
    message(STATUS "Configuring Windows installation targets...")

    if(TARGET Qt5::windeployqt)
        get_target_property(WINDEPLOYQT_EXE Qt5::windeployqt IMPORTED_LOCATION)
        set(WINDEPLOYQT_TARGET "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${PROJECT_DISPLAY_NAME}.exe")
        set(WINDEPLOYQT_TMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/windeployqt")
        string(
            JOIN " " WINDEPLOYQT_COMMANDS
            COMMAND "\"${CMAKE_COMMAND}\"" -E remove_directory "\"${WINDEPLOYQT_TMP_DIR}\""
            COMMAND "\"${CMAKE_COMMAND}\"" -E make_directory "\"${WINDEPLOYQT_TMP_DIR}\""
            COMMAND "\"${WINDEPLOYQT_EXE}\"" --no-translations --dir "\"${WINDEPLOYQT_TMP_DIR}\"" "\"${WINDEPLOYQT_TARGET}\""
        )

        # execute windeployqt in a tmp directory
        install(CODE "execute_process(${WINDEPLOYQT_COMMANDS})")

        # copy deployment directory during installation
        install(
            DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/windeployqt/" # trailing slash is important!
            DESTINATION bin
        )
    endif()

    include(InstallRequiredSystemLibraries)
    install(
        PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
        DESTINATION bin
    )

    install(
        TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION bin/static
    )
else() # unknown
    message(WARNING "Can't detect platform. Installation targets are not configured!")
endif()

# translation stuff
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

file(GLOB TARGET_TRANSLATIONS ${CMAKE_CURRENT_LIST_DIR/languages/*.ts})

qt5_create_translation(TRANSLATION_MESSAGES ${FILES_TO_TRANSLATE} ${TARGET_TRANSLATIONS})
qt5_add_translation(TRANSLATION_QM ${TARGET_TRANSLATIONS})
add_custom_target(translations_update DEPENDS ${TRANSLATION_MESSAGES})
add_custom_target(translations DEPENDS ${TRANSLATION_QM})

if(APPLE AND UNIX) ## OSX
	install(FILES ${TRANSLATION_QM} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_DISPLAY_NAME}.app/Contents/Resources/translations")
else()
	install(FILES ${TRANSLATION_QM} DESTINATION translations)
endif()
