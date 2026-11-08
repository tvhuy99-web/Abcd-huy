# This fork ships the Vietnamese catalog and English source strings.
set(LANGUAGES "vi")

message(STATUS "Process LANGUAGES variable --")

if(NOT GETTEXT_MSGFMT_EXECUTABLE)
    message(FATAL_ERROR
            "Gettext not found. Install gettext package or disable \
             localization with \"-DLOCALIZE=OFF\". \
             See doc/COMPILING/COMPILING-CMAKE.md for details and more info.")
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

foreach (LANG ${LANGUAGES})
    message(VERBOSE "Add translation for ${LANG}: ${LANG}.po")
endforeach ()

cmake_path(GET GETTEXT_MSGFMT_EXECUTABLE PARENT_PATH _path)
find_program(XGETTEXT_MSGFMT_EXECUTABLE
    xgettext
    HINTS
        ${_path}
)
if(NOT ${XGETTEXT_MSGFMT_EXECUTABLE})
    set(XGETTEXT_MSGFMT_EXECUTABLE xgettext)
endif()

# Generate cataclysm-dda.pot
add_custom_target(
        translations
        COMMAND ${CMAKE_SOURCE_DIR}/lang/update_pot.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

foreach (LANG ${LANGUAGES})
    set(_PO ${CMAKE_SOURCE_DIR}/lang/po/${LANG}.po)
    if(NOT EXISTS ${_PO})
        set(_PO ${CMAKE_SOURCE_DIR}/lang/po/${LANG}.po.gz)
    endif()
    set(_MO ${CMAKE_SOURCE_DIR}/lang/mo/${LANG}/LC_MESSAGES/cataclysm-dda.mo)
    set(_PO_DEPENDENCIES ${_PO} ${CMAKE_SOURCE_DIR}/lang/compile_mo.py)
    set(_OVERLAY ${CMAKE_SOURCE_DIR}/lang/po/accessibility/${LANG}.po)
    if(EXISTS ${_OVERLAY})
        list(APPEND _PO_DEPENDENCIES ${_OVERLAY})
    endif()
    add_custom_command(
            OUTPUT ${_MO}
            COMMAND ${CMAKE_COMMAND} -E
                make_directory ${CMAKE_SOURCE_DIR}/lang/mo/${LANG}/LC_MESSAGES
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/lang/compile_mo.py
                ${_PO} ${_MO} ${GETTEXT_MSGFMT_EXECUTABLE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${_PO_DEPENDENCIES}
    )
    list(APPEND _MO_FILES ${_MO})
    if (RELEASE)
        if(USE_PREFIX_DATA_DIR)
            install(DIRECTORY ${CMAKE_SOURCE_DIR}/lang/mo/${LANG}
                TYPE LOCALE)
        else()
            install(DIRECTORY ${CMAKE_SOURCE_DIR}/lang/mo/${LANG}
                DESTINATION lang/mo)
        endif()
    endif ()
endforeach()

# Auto-Compile translation on release builds only
if (RELEASE)
    add_custom_target(locale ALL DEPENDS ${_MO_FILES})
else()
    add_custom_target(locale DEPENDS ${_MO_FILES})
endif()
