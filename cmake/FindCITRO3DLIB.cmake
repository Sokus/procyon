# - Try to find citro3dlib
# Once done this will define
#  LIBCITRO3D_FOUND - System has ctrulib
#  LIBCITRO3D_INCLUDE_DIRS - The ctrulib include directories
#  LIBCITRO3D_LIBRARIES - The libraries needed to use ctrulib
#
# It also adds an imported target named `3ds-citro3d`.
# Linking it is the same as target_link_libraries(target ${LIBCITRO3D_LIBRARIES})
# and target_include_directories(target ${LIBCITRO3D_INCLUDE_DIRS})

# DevkitPro paths are broken on windows, so we have to fix those
macro(msys_to_cmake_path MsysPath ResultingPath)
    string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" ${ResultingPath} "${MsysPath}")
endmacro()

if(NOT DEVKITPRO)
    msys_to_cmake_path("$ENV{DEVKITPRO}" DEVKITPRO)
endif()

set(CITRO3DLIB_PATHS
    $ENV{CITRO3DLIB}
    libcitro3d
    citro3dlib
    ${DEVKITPRO}/libctru
    ${DEVKITPRO}/ctrulib
)

find_path(LIBCITRO3D_INCLUDE_DIR citro3d.h
          PATHS ${CITRO3DLIB_PATHS}
          PATH_SUFFIXES include libctru/include
)

find_library(LIBCITRO3D_LIBRARY NAMES citro3d libcitro3d.a
          PATHS ${CITRO3DLIB_PATHS}
          PATH_SUFFIXES lib libctru/lib
)

set(LIBCITRO3D_LIBRARIES ${LIBCITRO3D_LIBRARY})
set(LIBCITRO3D_INCLUDE_DIRS ${LIBCITRO3D_INCLUDE_DIR})

# handle the QUIETLY and REQUIRED arguments and set LIBCTRU_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CITRO3DLIB  DEFAULT_MSG
                                  LIBCITRO3D_LIBRARY LIBCITRO3D_INCLUDE_DIR
)

mark_as_advanced(LIBCITRO3D_INCLUDE_DIR LIBCITRO3D_LIBRARY )

if(CITRO3DLIB_FOUND)
    set(CITRO3DLIB ${LIBCITRO3D_INCLUDE_DIR}/..)
    message(STATUS "setting CITRO3DLIB to ${CITRO3DLIB}")

    add_library(3ds-citro3d STATIC IMPORTED GLOBAL)
    set_target_properties(3ds-citro3d PROPERTIES
        IMPORTED_LOCATION "${LIBCITRO3D_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBCITRO3D_INCLUDE_DIR}"
    )
    target_link_libraries(3ds-citro3d INTERFACE 3ds-ctru)
endif()
