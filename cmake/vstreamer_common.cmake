# Common things for sdk and sdk unittests.

#include("ugcs/common")

# Find all platform dependent source files in the specified SDK directory.
# include directory list is placed in INCLUDES_VAR
# Source file list is placed in SOURCES_VAR
function(Find_platform_sources VST_DIR INCLUDES_VAR SOURCES_VAR)
    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        file(GLOB PLAT_SOURCES
                "${VST_DIR}/src/platform/linux/*.h"
                "${VST_DIR}/src/platform/linux/*.cpp"
                "${VST_DIR}/src/platform/unix/*.h"
                "${VST_DIR}/src/platform/unix/*.cpp"
                )
        set (PLAT_INCLUDES
                "${VST_DIR}/include/platform/linux"
                "${VST_DIR}/include/platform/unix"
                )
    elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
        file(GLOB PLAT_SOURCES
                "${VST_DIR}/src/platform/mac/*.h"
                "${VST_DIR}/src/platform/mac/*.cpp"
                "${VST_DIR}/src/platform/unix/*.h"
                "${VST_DIR}/src/platform/unix/*.cpp"
                )
        set (PLAT_INCLUDES
                "${VST_DIR}/include/platform/mac"
                "${VST_DIR}/include/platform/unix"
                )
    elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
        file(GLOB PLAT_SOURCES
                "${VST_DIR}/src/platform/win/*.h"
                "${VST_DIR}/src/platform/win/*.cpp"
                )
        set (PLAT_INCLUDES
                "${VST_DIR}/include/platform/win"
                )
        # MinGW does not have bfd.h in its standard includes, so hack it a bit.
        #find_path(BFD_INCLUDE "bfd.h" PATH_SUFFIXES "..//include")
        #set (PLAT_INCLUDES ${PLAT_INCLUDES} ${BFD_INCLUDE})
    endif()

    set(${SOURCES_VAR} ${PLAT_SOURCES} PARENT_SCOPE)
    set(${INCLUDES_VAR} ${PLAT_INCLUDES} PARENT_SCOPE)
endfunction()


# Function for generating DLL import libraries from .def files in the specified
# directory. Global variable DLL_IMPORT_LIBS contains all the import libraries
# generated, DLL_IMPORT_LIB_NAMES - short names for generated libraries
#function(Process_dll_defs DIR_PATH)
#    if (CMAKE_SYSTEM_NAME MATCHES "Windows" AND ENABLE_DLL_IMPORT)
#        file(GLOB DEFS "${DIR_PATH}/*.def")
#        foreach(DEF ${DEFS})
#            get_filename_component(BASENAME ${DEF} NAME_WE)
#            set(LIBNAME ${CMAKE_BINARY_DIR}/lib${BASENAME}.a)
#            add_custom_command(OUTPUT
#                ${LIBNAME}
#                COMMAND dlltool -k -d ${DEF} -l ${LIBNAME}
#                DEPENDS ${DEF})
#            set(DLL_IMPORT_LIBS ${DLL_IMPORT_LIBS} ${LIBNAME} PARENT_SCOPE)
#            set(LIB_NAMES ${LIB_NAMES} ${LIBNAME})
#            set(DLL_IMPORT_LIB_NAMES ${DLL_IMPORT_LIB_NAMES} ${BASENAME} PARENT_SCOPE)
#        endforeach()
#        set(VSM_PLAT_LIBS ${VSM_PLAT_LIBS} ${LIB_NAMES} PARENT_SCOPE)
#    endif()
#endfunction()


