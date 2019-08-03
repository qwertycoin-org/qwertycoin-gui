set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "${PROJECT_VENDOR_NAME}")
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_BINARY_DIR}/dist")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_VENDOR_URL}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-v${PROJECT_VERSION}-${PROJECT_OS_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_DISPLAY_NAME}\${PROJECT_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_STRIP_FILES ON)

if(PROJECT_OS_LINUX OR PROJECT_OS_POSIX) # Linux
    message(STATUS "Configuring Linux (or POSIX) package...")

    set(CPACK_GENERATOR DEB RPM)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/qwertycoin-gui")

    # DEB
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <dev@qwertycoin.org>")
    set(CPACK_DEBIAN_PACKAGE_SECTION Office)
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)

    # RPM
    set(CPACK_RPM_PACKAGE_VENDOR "${CPACK_PACKAGE_VENDOR} <dev@qwertycoin.org>")
    set(CPACK_RPM_PACKAGE_GROUP Office)
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_COMPRESSION_TYPE "gzip")
elseif(PROJECT_OS_MACOS) # macOS
    message(STATUS "Configuring macOS package...")

    set(CPACK_GENERATOR DragNDrop)
    # TODO: Configure additional variables
    #   - CPACK_DMG_DS_STORE
    #   - CPACK_DMG_BACKGROUND_IMAGE
elseif(PROJECT_OS_WINDOWS) # Windows
    message(STATUS "Configuring Windows package...")
    set(CPACK_GENERATOR NSIS)
    set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_DISPLAY_NAME}")
    set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_DISPLAY_NAME}")
    set(CPACK_NSIS_HELP_LINK "${PROJECT_VENDOR_URL}")
    set(CPACK_NSIS_URL_INFO_ABOUT "${PROJECT_VENDOR_URL}")
    set(CPACK_NSIS_CONTACT "${PROJECT_VENDOR_NAME} <dev@qwertycoin.org>")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${PROJECT_DISPLAY_NAME}.exe")
else() # unknown
    message(WARNING "Can't detect platform. Package settings are not initialized!")
endif()

include(CPack)
