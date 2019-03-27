# Qt5

set(CMAKE_INCLUDE_CURRENT_DIR ON) # Find includes in corresponding build directories (for moc tool)
set(CMAKE_AUTOMOC ON) # Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOUIC OFF) # Create code from a list of Qt designer ui files
set(CMAKE_AUTORCC ON) # Automatically handle Qt resource files

set(Qt5_COMPONENTS Core Gui Network Widgets)
if(APPLE)
    list(APPEND Qt5_COMPONENTS PrintSupport)
endif()

if(EXISTS "$ENV{Qt5_DIR}" AND IS_DIRECTORY "$ENV{Qt5_DIR}")
    message(STATUS "Qt5_DIR is set. Using precompiled Qt5.")
    message(STATUS "Qt5_DIR path: $ENV{Qt5_DIR}")
    find_package(Qt5 REQUIRED COMPONENTS ${Qt5_COMPONENTS})
else()
    message(STATUS "Qt5_DIR is not set. Using Hunter (package manager) to install Qt5.")
    hunter_add_package(Qt COMPONENTS ${Qt5_COMPONENTS})
    find_package(Qt5 QUIET REQUIRED COMPONENTS ${Qt5_COMPONENTS})
endif()
