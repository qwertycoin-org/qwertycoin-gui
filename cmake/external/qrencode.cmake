# qrencode

set(qrencode_CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    -DWITH_TOOLS:BOOL=FALSE
)

if(CMAKE_TOOLCHAIN_FILE)
    list(INSERT qrencode_CMAKE_ARGS 0 -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif()

if(NOT WIN32)
    list(INSERT qrencode_CMAKE_ARGS 0 -DCMAKE_BUILD_TYPE=Release)
endif()

ExternalProject_Add(qrencode
    GIT_REPOSITORY https://github.com/fukuchi/libqrencode.git
    GIT_TAG master
    GIT_SHALLOW ON
    GIT_PROGRESS OFF

    UPDATE_COMMAND ""
    PATCH_COMMAND ""

    CMAKE_ARGS ${qrencode_CMAKE_ARGS}

    # CONFIGURE_COMMAND (use default)
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    BUILD_ALWAYS FALSE
    TEST_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release --target install
)

ExternalProject_Get_property(qrencode INSTALL_DIR)
set(qrencode_INCLUDE_DIRS
    "${INSTALL_DIR}/include"
)
set(qrencode_LIB
    "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}qrencode${CMAKE_STATIC_LIBRARY_SUFFIX}"
)
mark_as_advanced(qrencode_INCLUDE_DIRS qrencode_LIB)

if(NOT EXISTS "${INSTALL_DIR}/include")
    file(MAKE_DIRECTORY "${INSTALL_DIR}/include")
endif()

add_library(qrencode::qrencode STATIC IMPORTED GLOBAL)
set_target_properties(qrencode::qrencode PROPERTIES
    IMPORTED_CONFIGURATIONS Release
    IMPORTED_LOCATION_RELEASE ${qrencode_LIB}
    #INTERFACE_INCLUDE_DIRECTORIES "${qrencode_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES qrencode::qrencode
)
add_dependencies(qrencode::qrencode qrencode)
