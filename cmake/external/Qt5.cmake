# Qt5

set(CMAKE_INCLUDE_CURRENT_DIR ON) # Find includes in corresponding build directories (for moc tool)
set(CMAKE_AUTOMOC ON) # Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOUIC OFF) # Create code from a list of Qt designer ui files
set(CMAKE_AUTORCC ON) # Automatically handle Qt resource files

set(Qt5_COMPONENTS Core Gui LinguistTools Network Widgets)
if (APPLE)
    list(APPEND Qt5_COMPONENTS PrintSupport)
endif ()

#if (EXISTS "$ENV{Qt5_DIR}" AND IS_DIRECTORY "$ENV{Qt5_DIR}")
    message(STATUS "Qt5_DIR is set. Using precompiled Qt5.")
    message(STATUS "Qt5_DIR path: $ENV{Qt5_DIR}")
    find_package(Qt5 REQUIRED COMPONENTS ${Qt5_COMPONENTS})
#else ()
#    message(STATUS "Qt5_DIR is not set. Using Hunter (package manager) to install Qt5.")
#    hunter_add_package(Qt)
#    find_package(Qt5 QUIET REQUIRED COMPONENTS ${Qt5_COMPONENTS})
#endif ()

if(TARGET Qt5::qmake)
    get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(QT_INSTALL_BINS "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    get_filename_component(QT_INSTALL_LIBS "${QT_INSTALL_BINS}/../lib" ABSOLUTE)
    message(STATUS "QT_QMAKE_EXECUTABLE: ${QT_QMAKE_EXECUTABLE}")
    message(STATUS "QT_INSTALL_BINS: ${QT_INSTALL_BINS}")
    message(STATUS "QT_INSTALL_LIBS: ${QT_INSTALL_LIBS}")

    if(APPLE AND EXISTS "${QT_INSTALL_BINS}/macdeployqt")
        add_executable(Qt5::macdeployqt IMPORTED)

        set_target_properties(
            Qt5::macdeployqt
            PROPERTIES IMPORTED_LOCATION "${QT_INSTALL_BINS}/macdeployqt"
        )
        set(Qt5_macdeployqt_FOUND TRUE)

        message(STATUS "Found macdeployqt ${QT_INSTALL_BINS}/macdeployqt")
    elseif(WIN32)
        add_executable(Qt5::windeployqt IMPORTED)

        set_target_properties(
            Qt5::windeployqt
            PROPERTIES IMPORTED_LOCATION "${QT_INSTALL_BINS}/windeployqt.exe"
        )
        set(Qt5_windeployqt_FOUND TRUE)

        message(STATUS "Found windeployqt ${QT_INSTALL_BINS}/windeployqt.exe")
    else()
        # This is not an error! Android, iOS and Linux don't have this tool.
        message(STATUS "Qt5 deployment tool (macdeployqt or windeployqt) is not found.")
    endif()
endif()
