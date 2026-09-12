add_subdirectory(QR-Code-scanner)
add_subdirectory(openpgp)
add_subdirectory(zxcvbn-c)

qt5_add_resources(RESOURCES ../qml.qrc)

# Compile source files (.h/.cpp)
file(GLOB SOURCE_FILES
	"*.h"
	"*.cpp"
    "main/*.h"
    "main/*.cpp"
    "libwalletqt/WalletManager.cpp"
    "libwalletqt/WalletListenerImpl.cpp"
    "libwalletqt/Wallet.cpp"
    "libwalletqt/PassphraseHelper.cpp"
    "libwalletqt/PendingTransaction.cpp"
    "libwalletqt/TransactionHistory.cpp"
    "libwalletqt/TransactionInfo.cpp"
    "libwalletqt/QRCodeImageProvider.cpp"
    "libwalletqt/AddressBook.cpp"
    "libwalletqt/Subaddress.cpp"
    "libwalletqt/SubaddressAccount.cpp"
    "libwalletqt/UnsignedTransaction.cpp"
    "libwalletqt/WalletManager.h"
    "libwalletqt/Wallet.h"
    "libwalletqt/PassphraseHelper.h"
    "libwalletqt/PendingTransaction.h"
    "libwalletqt/TransactionHistory.h"
    "libwalletqt/TransactionInfo.h"
    "libwalletqt/QRCodeImageProvider.h"
    "libwalletqt/Transfer.h"
    "libwalletqt/AddressBook.h"
    "libwalletqt/Subaddress.h"
    "libwalletqt/SubaddressAccount.h"
    "libwalletqt/UnsignedTransaction.h"
    "daemon/*.h"
    "daemon/*.cpp"
    "p2pool/*.h"
    "p2pool/*.cpp"
    "model/*.h"
    "model/*.cpp"
    "qt/*.h"
    "qt/*.cpp"
)
if(QML_TESTS)
    list(APPEND SOURCE_FILES "${CMAKE_SOURCE_DIR}/tests/qml/QmlTestHarness.cpp")
endif()
if(APPLE)
    list(APPEND SOURCE_FILES "qt/macoshelper.mm")
endif()

set(EXECUTABLE_FLAG)
if(MINGW)
    set(EXECUTABLE_FLAG WIN32)

    set(ICON ${PROJECT_SOURCE_DIR}/images/appicon.ico)
    set(ICON_RC ${CMAKE_CURRENT_BINARY_DIR}/icon.rc)
    set(ICON_RES ${CMAKE_CURRENT_BINARY_DIR}/icon.o)
    file(WRITE ${ICON_RC} "IDI_ICON1 ICON DISCARDABLE \"${ICON}\"")
    find_program(Qt5_WINDRES_EXECUTABLE NAMES windres x86_64-w64-mingw32-windres REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
    add_custom_command(OUTPUT ${ICON_RES} COMMAND ${Qt5_WINDRES_EXECUTABLE} ${ICON_RC} ${ICON_RES} MAIN_DEPENDENCY ${ICON_RC})
    list(APPEND RESOURCES ${ICON_RES})
endif()

if(APPLE)
    set(ICON ${PROJECT_SOURCE_DIR}/images/appicon.icns)
    set_source_files_properties(${ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    list(APPEND RESOURCES ${ICON})
endif()

set(qwertycoin_gui_sources
    ${SOURCE_FILES}
    ${RESOURCES}
)

if(NOT ANDROID)
    add_executable(qwertycoin-gui ${EXECUTABLE_FLAG} ${qwertycoin_gui_sources})
else()
    add_library(qwertycoin-gui SHARED ${qwertycoin_gui_sources})
    set_target_properties(qwertycoin-gui PROPERTIES COMPILE_DEFINITIONS "ANDROID")
endif()

set_target_properties(qwertycoin-gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/share/Info.plist"
)

# OpenGL
target_include_directories(qwertycoin-gui PUBLIC ${OPENGL_INCLUDE_DIR})
message(STATUS "OpenGL: include dir at ${OPENGL_INCLUDE_DIR}")
message(STATUS "OpenGL: libraries at ${OPENGL_LIBRARIES}")

target_include_directories(qwertycoin-gui PUBLIC ${Qt5Gui_PRIVATE_INCLUDE_DIRS})

target_include_directories(qwertycoin-gui PUBLIC
	${CMAKE_SOURCE_DIR}/qwertycoin/include
    ${CMAKE_SOURCE_DIR}/qwertycoin/src
    ${CMAKE_SOURCE_DIR}/qwertycoin/external/easylogging++
    ${CMAKE_SOURCE_DIR}/qwertycoin/contrib/epee/include
    ${CMAKE_SOURCE_DIR}/qwertycoin/external/qrcodegen
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/daemon
    ${CMAKE_CURRENT_SOURCE_DIR}/p2pool
	${CMAKE_CURRENT_SOURCE_DIR}/libwalletqt
	${CMAKE_CURRENT_SOURCE_DIR}/model
	${CMAKE_CURRENT_SOURCE_DIR}/QR-Code-scanner
	${CMAKE_CURRENT_SOURCE_DIR}/zxcvbn-c
)

target_compile_definitions(qwertycoin-gui
	PUBLIC
	${Qt5Widgets_DEFINITIONS}
    ${Qt5Qml_DEFINITIONS}
)

if(QML_TESTS)
    target_include_directories(qwertycoin-gui PRIVATE "${CMAKE_SOURCE_DIR}/tests/qml")
    target_compile_definitions(qwertycoin-gui PRIVATE
        QML_TESTS
        QML_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}/tests/qml"
    )
endif()

if(APPLE)
    if(NOT ICU_ROOT)
        execute_process(COMMAND brew --prefix icu4c OUTPUT_VARIABLE ICU_ROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
    find_package(ICU REQUIRED COMPONENTS data i18n uc)
    target_link_directories(qwertycoin-gui PRIVATE ${ICU_ROOT}/lib)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

target_link_libraries(qwertycoin-gui
    epee
    common
    net
    epose
    wallet_api
    qrcodegen
    easylogging
    Qt5::Network
    Qt5::Concurrent
    ${QT5_LIBRARIES}
    ${EXTRA_LIBRARIES}
    openpgp
    qrdecoder
    translations
    zxcvbn
)

if(WITH_SCANNER)
    target_link_libraries(qwertycoin-gui qrscanner)
    if(LINUX AND NOT ANDROID)
        target_link_libraries(qwertycoin-gui
            jpeg
            v4l2
            v4lconvert
            rt
        )
    endif()
endif()

add_custom_command(TARGET qwertycoin-gui POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:daemon> $<TARGET_FILE_DIR:qwertycoin-gui>)

include(Deploy)

install(TARGETS qwertycoin-gui
    DESTINATION bin
)

if(QML_TESTS)
    add_executable(qwertycoin-gui-epose-tests
        "${CMAKE_SOURCE_DIR}/tests/epose/tst_EposeIntegration.cpp"
        daemon/DaemonManager.cpp
        qt/EposeManager.cpp
        qt/FutureScheduler.cpp
    )
    set_target_properties(qwertycoin-gui-epose-tests PROPERTIES
        AUTOMOC ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    target_include_directories(qwertycoin-gui-epose-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/daemon"
        "${CMAKE_SOURCE_DIR}/qwertycoin/src"
        "${CMAKE_SOURCE_DIR}/qwertycoin/contrib/epee/include"
        "${CMAKE_SOURCE_DIR}/qwertycoin/external/easylogging++"
    )
    target_link_libraries(qwertycoin-gui-epose-tests
        epee
        common
        net
        epose
        Qt5::Core
        Qt5::Network
        Qt5::Concurrent
        Qt5::Qml
        Qt5::Test
        Qt5::Widgets
        ${EXTRA_LIBRARIES}
    )
    add_test(NAME qwertycoin-gui-epose-tests
             COMMAND qwertycoin-gui-epose-tests)
endif()
