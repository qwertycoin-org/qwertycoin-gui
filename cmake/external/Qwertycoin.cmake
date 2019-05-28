# Qwertycoin

set(Qwertycoin_CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DBUILD_ALL:BOOL=FALSE) # Only libraries are required!

if(CMAKE_TOOLCHAIN_FILE)
    list(INSERT Qwertycoin_CMAKE_ARGS 0 -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif()

if(NOT WIN32)
    list(INSERT Qwertycoin_CMAKE_ARGS 0 -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
endif()

ExternalProject_Add(Qwertycoin
    GIT_REPOSITORY https://github.com/qwertycoin-org/qwertycoin.git
    GIT_TAG master
    GIT_SHALLOW ON
    GIT_PROGRESS OFF

    UPDATE_COMMAND ""
    PATCH_COMMAND ""

    CMAKE_ARGS ${Qwertycoin_CMAKE_ARGS}

    # CONFIGURE_COMMAND (use default)
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target QwertycoinFramework
    BUILD_ALWAYS FALSE
    TEST_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_property(Qwertycoin SOURCE_DIR)
ExternalProject_Get_property(Qwertycoin BINARY_DIR)

set(QwertycoinFramework_INCLUDE_DIRS
    "${SOURCE_DIR}/include"
    "${SOURCE_DIR}/lib"
    "${BINARY_DIR}/lib"
    "${BINARY_DIR}/_ExternalProjects/Install/sparsehash/include"
)
if(MSVC)
    list(APPEND QwertycoinFramework_INCLUDE_DIRS "${SOURCE_DIR}/lib/Platform/Windows")
    list(APPEND QwertycoinFramework_INCLUDE_DIRS "${SOURCE_DIR}/lib/Platform/msc")
elseif(APPLE)
    list(APPEND QwertycoinFramework_INCLUDE_DIRS "${SOURCE_DIR}/lib/Platform/OSX")
    list(APPEND QwertycoinFramework_INCLUDE_DIRS "${SOURCE_DIR}/lib/Platform/Posix")
elseif(ANDROID)
    list(APPEND QwertycoinFramework_INCLUDE_DIRS "${SOURCE_DIR}/lib/Platform/Android")
    list(APPEND QwertycoinFramework_INCLUDE_DIRS "${SOURCE_DIR}/lib/Platform/Posix")
else()
    list(APPEND QwertycoinFramework_INCLUDE_DIRS "${SOURCE_DIR}/lib/Platform/Linux")
    list(APPEND QwertycoinFramework_INCLUDE_DIRS "${SOURCE_DIR}/lib/Platform/Posix")
endif()

set(QwertycoinFramework_LIBS
    BlockchainExplorer
    Common
    Crypto
    CryptoNoteCore
    CryptoNoteProtocol
    Http
    InProcessNode
    JsonRpcServer
    Logging
    Mnemonics
    NodeRpcProxy
    P2p
    PaymentGate
    Rpc
    Serialization
    System
    Transfers
    Wallet
)

mark_as_advanced(QwertycoinFramework_INCLUDE_DIRS QwertycoinFramework_LIBS)

foreach(LIB ${QwertycoinFramework_LIBS})
    set(LIB_IMPORTED_LOCATION_KEY "RELEASE")
    set(LIB_SUFFIX "")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(LIB_IMPORTED_LOCATION_KEY "DEBUG")
        set(LIB_SUFFIX "d")
    endif()

    set(${LIB}_filename "${CMAKE_STATIC_LIBRARY_PREFIX}QwertycoinFramework_${LIB}${LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    if(WIN32)
        set(${LIB}_library "${BINARY_DIR}/lib/${CMAKE_BUILD_TYPE}/${${LIB}_filename}")
    else()
        set(${LIB}_library "${BINARY_DIR}/lib/${${LIB}_filename}")
    endif()

    add_library(QwertycoinFramework::${LIB} UNKNOWN IMPORTED GLOBAL)
    set_target_properties(QwertycoinFramework::${LIB} PROPERTIES
        IMPORTED_CONFIGURATIONS ${CMAKE_BUILD_TYPE}
        IMPORTED_LOCATION_${LIB_IMPORTED_LOCATION_KEY} ${${LIB}_library}
    )
    add_dependencies(QwertycoinFramework::${LIB} Qwertycoin)
endforeach()

# QwertycoinFramework::Common
target_link_libraries(QwertycoinFramework::Common INTERFACE
    Threads::Threads
    QwertycoinFramework::Crypto
)
if(UNIX AND NOT ANDROID)
    target_link_libraries(QwertycoinFramework::Common INTERFACE -lresolv)
endif()

# QwertycoinFramework::CryptoNoteCore
target_link_libraries(QwertycoinFramework::CryptoNoteCore INTERFACE
    Boost::filesystem
    Boost::program_options
    QwertycoinFramework::BlockchainExplorer
    QwertycoinFramework::Common
    QwertycoinFramework::Crypto
    QwertycoinFramework::Logging
    QwertycoinFramework::Serialization
)

# QwertycoinFramework::InProcessNode
target_link_libraries(QwertycoinFramework::InProcessNode INTERFACE
    QwertycoinFramework::BlockchainExplorer
)

# QwertycoinFramework::NodeRpcProxy
target_link_libraries(QwertycoinFramework::NodeRpcProxy INTERFACE
    QwertycoinFramework::System
)

# QwertycoinFramework::P2p
target_link_libraries(QwertycoinFramework::P2p INTERFACE
    MiniUPnP::miniupnpc
    QwertycoinFramework::CryptoNoteProtocol
)

if(WIN32 AND MSVC)
    target_link_libraries(QwertycoinFramework::P2p INTERFACE Bcrypt)
endif()

# QwertycoinFramework::PaymentGate
target_link_libraries(QwertycoinFramework::PaymentGate INTERFACE
    QwertycoinFramework::JsonRpcServer
    QwertycoinFramework::NodeRpcProxy
    QwertycoinFramework::System
    QwertycoinFramework::Wallet
)

# QwertycoinFramework::Rpc
target_link_libraries(QwertycoinFramework::Rpc INTERFACE
    QwertycoinFramework::BlockchainExplorer
    QwertycoinFramework::CryptoNoteCore
    QwertycoinFramework::P2p
    QwertycoinFramework::PaymentGate
    QwertycoinFramework::Serialization
    QwertycoinFramework::System
    QwertycoinFramework::Http
)

# QwertycoinFramework::Serialization
target_link_libraries(QwertycoinFramework::Serialization INTERFACE
    QwertycoinFramework::Common
)

# QwertycoinFramework::System
if(WIN32)
    target_link_libraries(QwertycoinFramework::System INTERFACE ws2_32)
endif()

# QwertycoinFramework::Transfers
target_link_libraries(QwertycoinFramework::Transfers INTERFACE
    QwertycoinFramework::CryptoNoteCore
)

# QwertycoinFramework::Wallet
target_link_libraries(QwertycoinFramework::Wallet INTERFACE
    Boost::filesystem
    QwertycoinFramework::Common
    QwertycoinFramework::Crypto
    QwertycoinFramework::CryptoNoteCore
    QwertycoinFramework::Logging
    QwertycoinFramework::Mnemonics
    QwertycoinFramework::Rpc
    QwertycoinFramework::Transfers
)

# MiniUPnP::miniupnpc

get_filename_component(MINIUPNP_DIR "${BINARY_DIR}/_ExternalProjects/Install/MiniUPnP" ABSOLUTE CACHE)
set(MINIUPNP_INCLUDE_DIRS "${MINIUPNP_DIR}/include")
set(MINIUPNP_STATIC_LIBRARY "${MINIUPNP_DIR}/lib/libminiupnpc${CMAKE_STATIC_LIBRARY_SUFFIX}")
mark_as_advanced(MINIUPNP_DIR MINIUPNP_INCLUDE_DIRS MINIUPNP_STATIC_LIBRARY)

add_library(MiniUPnP::miniupnpc STATIC IMPORTED GLOBAL)
add_dependencies(MiniUPnP::miniupnpc MiniUPnP)
set_target_properties(MiniUPnP::miniupnpc PROPERTIES
    IMPORTED_LOCATION "${MINIUPNP_STATIC_LIBRARY}"
    INTERFACE_COMPILE_DEFINITIONS "MINIUPNP_STATICLIB"
)
if(WIN32)
    target_link_libraries(MiniUPnP::miniupnpc INTERFACE iphlpapi mswsock ws2_32)
endif()
