set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "${PROJECT_VENDOR_NAME}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/dist")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_VENDOR_URL}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-v${PROJECT_VERSION}-${PROJECT_OS_NAME}")
set(CPACK_PACKAGE_CONTACT "https://qwertycoin.org")
set(CPACK_STRIP_FILES ON)

if(UNIX AND NOT APPLE AND NOT ANDROID) # linux
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/qwertycoin")

    if(NOT RPMBUILD)
        set(CPACK_GENERATOR DEB)
        set(CPACK_SYSTEM_NAME 64-bit)
        set(CPACK_DEBIAN_PACKAGE_NAME ${CPACK_PACKAGE_NAME})
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <dev@qwertycoin.org>")
        set(CPACK_DEBIAN_PACKAGE_SECTION Office)
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
        set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Qwertycoin QWC wallet
 Qwertycoin is a decentralized, privacy oriented peer-to-peer
 cryptocurrency. It is open-source, nobody owns or controls Qwertycoin
 and everyone can take part.")
    else()
        set(CPACK_GENERATOR RPM)
        set(CPACK_SYSTEM_NAME x86_64)
        set(CPACK_RPM_PACKAGE_RELEASE ${PROJECT_GIT_COMMIT_ID})
        set(CPACK_RPM_PACKAGE_LICENSE "MIT")
        set(CPACK_RPM_PACKAGE_GROUP Office)
        set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase >= 5.3.2, qt5-qtbase-gui >= 5.3.2")
        set(CPACK_RPM_PACKAGE_SUMMARY "Qwertycoin QWC wallet")
        set(CPACK_RPM_PACKAGE_DESCRIPTION "Qwertycoin QWC wallet
 Qwertycoin is a decentralized, privacy oriented peer-to-peer
 cryptocurrency. It is open-source, nobody owns or controls Qwertycoin
 and everyone can take part.")
    endif ()
elseif(APPLE) # macos
    set(CPACK_GENERATOR DragNDrop)
    set(MACOSX_BUNDLE_BUNDLE_NAME "Qwertycoin")
    set(MACOSX_BUNDLE_INFO_STRING "Qwertycoin GUI Wallet")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_ICON_FILE qwertycoin.icns)
elseif(WIN32) # windows
    set(CPACK_GENERATOR "ZIP")
endif()

include(CPack)
