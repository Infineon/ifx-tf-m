#-------------------------------------------------------------------------------
# SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors#
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

find_package(Git)

# Rebuilds a patch set from a pristine base, used as the recovery path of
# apply_patches() when the working tree has drifted (see below).
#
# It resets the whole library working tree to HEAD - discarding tracked
# modifications and removing untracked/ignored files (build-regenerated or
# patch-created) - and then applies every patch of the set in order from that
# pristine base. Resetting the whole tree (rather than only the files this set
# touches) is what makes stacked patch sets robust: see the note in
# apply_patches() about set ordering.
function(_apply_patches_rebuild_set WORKING_DIRECTORY PATCH_FILES)
    list(SORT PATCH_FILES ORDER ASCENDING)

    # Discard all tracked modifications.
    execute_process(COMMAND "${GIT_EXECUTABLE}" checkout -- .
        WORKING_DIRECTORY ${WORKING_DIRECTORY}
        RESULT_VARIABLE GIT_CHECKOUT_STATUS
        ERROR_QUIET OUTPUT_QUIET
    )
    if (NOT GIT_CHECKOUT_STATUS EQUAL 0)
        message(FATAL_ERROR "Failed to reset tracked files at ${WORKING_DIRECTORY}")
    endif()
    # Remove untracked and ignored files. -x is required because patch-created
    # or build-regenerated files may be listed in .gitignore.
    execute_process(COMMAND "${GIT_EXECUTABLE}" clean -fdx
        WORKING_DIRECTORY ${WORKING_DIRECTORY}
        RESULT_VARIABLE GIT_CLEAN_STATUS
        ERROR_QUIET OUTPUT_QUIET
    )
    if (NOT GIT_CLEAN_STATUS EQUAL 0)
        message(FATAL_ERROR "Failed to clean working tree at ${WORKING_DIRECTORY}")
    endif()

    # Apply the whole set in order from the pristine base.
    foreach(PATCH ${PATCH_FILES})
        execute_process(COMMAND "${GIT_EXECUTABLE}" apply --verbose "${PATCH}"
            WORKING_DIRECTORY ${WORKING_DIRECTORY}
            RESULT_VARIABLE PATCH_STATUS
            COMMAND_ECHO STDOUT
        )
        if (NOT PATCH_STATUS EQUAL 0)
            message(FATAL_ERROR "Failed to apply patch ${PATCH} at ${WORKING_DIRECTORY}")
        endif()
    endforeach()
endfunction()


# This function applies the given patches that are not applied yet.
#
# Each patch is first handled individually and idempotently:
#   1. If the patch can be reverted, it is already applied -> skip it.
#   2. Else, if it applies cleanly on the current tree -> apply it. This
#      preserves any unrelated/compatible local changes.
#   3. Else, the working tree has drifted from a state the set can apply onto.
#      This happens when the build regenerates files that a patch also provides
#      - e.g. the generated tf-psa-crypto wrappers added by
#      0001-Add-generated-files-to-the-source-tree.patch, which exist on a
#      reconfigure but no longer match the patch. In that case the library tree
#      is reset to HEAD and the *whole set* is reapplied from that pristine base
#      (see _apply_patches_rebuild_set) and the function returns.
#
# Reapplying the whole set (rather than resetting per patch) is required because
# several patches of a set may modify the same file: a per-patch reset would
# discard the contributions of earlier patches that are not revisited.
#
# This makes (re)configuration robust against working trees that already
# contain a subset of the patches (CMake/MTB incremental builds, runs
# interrupted between patches): tiers 1 and 2 keep already-applied or cleanly
# applicable patches untouched, and the tree is only reset when a patch genuinely
# cannot be applied otherwise.
#
# Stacked patch sets applied to the same library across several apply_patches()
# calls (e.g. the main tf-psa-crypto patches followed by the platform-specific
# ones) keep working because the sets are applied in a fixed order on every
# (re)configuration. When an earlier set has to reset the tree to HEAD, it wipes
# the later sets' changes too, but each later set is reapplied afterwards on the
# pristine base the earlier set rebuilt, so it cleanly applies (tier 2) and never
# needs to reset the tree itself.
#
# WORKING_DIRECTORY - working directory where patches should be applied.
# PATCH_FILES - list of patches. Patches are applied in alphabetical order.
function(apply_patches WORKING_DIRECTORY PATCH_FILES)
    # Apply patches in ascending (alphabetical) order.
    list(SORT PATCH_FILES ORDER ASCENDING)
    foreach(PATCH ${PATCH_FILES})
        # Tier 1 - already applied? It can be reverted -> skip.
        execute_process(COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${PATCH}"
            WORKING_DIRECTORY ${WORKING_DIRECTORY}
            RESULT_VARIABLE PATCH_ALREADY_APPLIED
            ERROR_QUIET OUTPUT_QUIET
        )
        if (PATCH_ALREADY_APPLIED EQUAL 0)
            message(STATUS "Patch already applied, skipping: ${PATCH}")
            continue()
        endif()

        # Tier 2 - applies cleanly on the current tree? -> apply as is.
        execute_process(COMMAND "${GIT_EXECUTABLE}" apply --check "${PATCH}"
            WORKING_DIRECTORY ${WORKING_DIRECTORY}
            RESULT_VARIABLE PATCH_APPLIES_CLEANLY
            ERROR_QUIET OUTPUT_QUIET
        )
        if (PATCH_APPLIES_CLEANLY EQUAL 0)
            execute_process(COMMAND "${GIT_EXECUTABLE}" apply --verbose "${PATCH}"
                WORKING_DIRECTORY ${WORKING_DIRECTORY}
                RESULT_VARIABLE PATCH_STATUS
                COMMAND_ECHO STDOUT
            )
            if (NOT PATCH_STATUS EQUAL 0)
                message(FATAL_ERROR "Failed to apply patch ${PATCH} at ${WORKING_DIRECTORY}")
            endif()
            continue()
        endif()

        # Tier 3 - the tree has drifted. Rebuild the whole set from a clean
        # base and stop (the rebuild applies every patch of the set).
        message(STATUS "Patch set drifted; rebuilding from clean base (triggered by ${PATCH})")
        _apply_patches_rebuild_set("${WORKING_DIRECTORY}" "${PATCH_FILES}")
        return()
    endforeach()
endfunction()


# Returns a repository URL and a reference to the commit used to checkout the repository.
#
# REPO_URL_VAR - name of variable which receives repository URL.
# TAG_VAR - name of variable which receives reference to commit.
function(_get_fetch_remote_properties REPO_URL_VAR TAG_VAR)
    # Parse arguments
    set(options "")
    set(oneValueArgs GIT_REPOSITORY GIT_TAG)
    set(multiValueArgs "")
    cmake_parse_arguments(PARSE_ARGV 2 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if (ARG_GIT_REPOSITORY)
        set(${REPO_URL_VAR} ${ARG_GIT_REPOSITORY} PARENT_SCOPE)
        set(${TAG_VAR} ${ARG_GIT_TAG} PARENT_SCOPE)
    endif()
endfunction()


# This function helps to handle options with an empty string values.
# There is a feature/bug in CMake that result in problem with the empty string arguments.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/16341 for details
#
# Arguments:
#   [in]  KEY              - option name
#   [out]  KEY_VAR         - name of variable that is set to ${KEY} on exit if value is not
#                            an empty string otherwise to the empty string.
#   [out]  VALUE_VAR       - name of variable that is set to option value for ${KEY}.
#   [in/out]  ARG_LIST_VAR - name of variable that holds list of key/value pairs - arguments.
#                            Function looks for key/value pair specified by ${KEY} variable in
#                            this list. Function removes key/value pair specified by ${KEY} on
#                            exit.
#
# Example #1:
#   # We have following key/options:
#   #  GIT_SUBMODULES  ""
#   #  BOO  "abc"
#   #  HEY  "hi"
#   set(ARGS    GIT_SUBMODULES "" BOO "abc" HEY "hi")
#   # Extract key/value for option "GIT_SUBMODULES"
#   extract_key_value(GIT_SUBMODULES GIT_SUBMODULES_VAR GIT_SUBMODULES_VALUE_VAR ARGS)
#   # ${GIT_SUBMODULES_VAR} is equal to ""
#   # ${GIT_SUBMODULES_VALUE_VAR} is equal to ""
#
# Example #2:
#   # We have following key/options:
#   #  GIT_SUBMODULES  "name"
#   #  BOO  "abc"
#   #  HEY  "hi"
#   set(ARGS    GIT_SUBMODULES "name" BOO "abc" HEY "hi")
#   # Extract key/value for option "GIT_SUBMODULES"
#   extract_key_value(GIT_SUBMODULES GIT_SUBMODULES_VAR GIT_SUBMODULES_VALUE_VAR ARGS)
#   # ${GIT_SUBMODULES_VAR} is equal to "GIT_SUBMODULES"
#   # ${GIT_SUBMODULES_VALUE_VAR} is equal to "name"
function(extract_key_value KEY KEY_VAR VALUE_VAR ARG_LIST_VAR)
    list(FIND ${ARG_LIST_VAR} ${KEY} KEY_INDEX)
    if(${KEY_INDEX} GREATER_EQUAL 0)
        # Variable has been set, remove KEY
        list(REMOVE_AT ${ARG_LIST_VAR} ${KEY_INDEX})

        # Validate that there is an option value in the list of arguments
        list(LENGTH ${ARG_LIST_VAR} ARG_LIST_LENGTH)
        if(${KEY_INDEX} GREATER_EQUAL ${ARG_LIST_LENGTH})
            message(FATAL_ERROR "Missing option value for ${KEY}")
        endif()

        # Get value
        list(GET ${ARG_LIST_VAR} ${KEY_INDEX} VALUE)

        # Remove value in the list
        list(REMOVE_AT ${ARG_LIST_VAR} ${KEY_INDEX})

        # Update argument list
        set(${ARG_LIST_VAR} ${${ARG_LIST_VAR}} PARENT_SCOPE)

        # Set KEY_VAR & VALUE_VAR
        set(${KEY_VAR} ${KEY} PARENT_SCOPE)
        set(${VALUE_VAR} ${VALUE} PARENT_SCOPE)
    else()
        # Variable is not defined, set KEY_VAR & VALUE_VAR to empty strings
        set(${KEY_VAR} "" PARENT_SCOPE)
        set(${VALUE_VAR} "" PARENT_SCOPE)
    endif()
endfunction()


# This function allows to fetch library from a remote repository or use a local
# library copy.
#
# You can specify location of directory with patches. Patches are applied in
# alphabetical order.
#
# Arguments:
# [in]     LIB_NAME <name> - library name
# [in/out] LIB_SOURCE_PATH_VAR <var> - name of variable which holds path to library source
#           or "DOWNLOAD" if sources should be fetched from the remote repository. This
#           variable is updated in case if library is downloaded. It will point
#           to the path where FetchContent_MakeAvailable will locate local library copy.
# [out]    LIB_BINARY_PATH_VAR <var> - optional name of variable which is updated to
#           directory intended for use as a corresponding build directory if
#           library is fetched from the remote repository.
# [in]     LIB_BASE_DIR <path>  - is used to set FETCHCONTENT_BASE_DIR.
# [in]     LIB_PATCH_DIR <path> - optional path to local folder which contains patches
#           that should be applied.
# [in]     LIB_FORCE_PATCH - optional boolean to control applying patches when the path
#            is a local folder instead of fetching from the remote repository.
#            Defaults to ON. Set to OFF to disable patching for local sources.
# [in]     GIT_REPOSITORY, GIT_TAG, ... - see https://cmake.org/cmake/help/latest/module/ExternalProject.html
#           for more details
#
# This function set CMP0097 to NEW if CMAKE_VERSION is greater or equal than 3.18.0.
# Because of https://gitlab.kitware.com/cmake/cmake/-/issues/20579 CMP0097 is
# non-functional until cmake 3.18.0.
# See https://cmake.org/cmake/help/latest/policy/CMP0097.html for more info.
function(fetch_remote_library)
    # Parse arguments
    set(options "")
    set(oneValueArgs LIB_NAME LIB_SOURCE_PATH_VAR LIB_BINARY_PATH_VAR LIB_BASE_DIR LIB_PATCH_DIR LIB_FORCE_PATCH)
    set(multiValueArgs FETCH_CONTENT_ARGS)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if(ARG_LIB_BASE_DIR)
        set(FETCHCONTENT_BASE_DIR "${ARG_LIB_BASE_DIR}")
    endif()

    if ("${${ARG_LIB_SOURCE_PATH_VAR}}" STREQUAL "DOWNLOAD")
        set(SOURCE_PATH_IS_DOWNLOAD TRUE)
        # Process arguments which can be an empty string
        # There is a feature/bug in CMake that result in problem with empty string arguments
        # See https://gitlab.kitware.com/cmake/cmake/-/issues/16341 for details
        extract_key_value(GIT_SUBMODULES GIT_SUBMODULES GIT_SUBMODULES_VALUE ARG_FETCH_CONTENT_ARGS)

        # Validate that there is no empty arguments to FetchContent_Declare
        LIST(FIND ARG_FETCH_CONTENT_ARGS "" EMPTY_VALUE_INDEX)
        if(${EMPTY_VALUE_INDEX} GREATER_EQUAL 0)
            # There is an unsupported empty string argument, FATAL ERROR!
            math(EXPR EMPTY_KEY_INDEX "${EMPTY_VALUE_INDEX} - 1")
            list(GET ARG_FETCH_CONTENT_ARGS ${EMPTY_KEY_INDEX} EMPTY_KEY)
            # TODO: Use extract_key_value if you have argument with empty value (see GIT_SUBMODULES above)
            message(FATAL_ERROR "fetch_remote_library: Unexpected empty string value for ${EMPTY_KEY}. "
                                "Please, validate arguments or update fetch_remote_library to support empty value for ${EMPTY_KEY}!!!")
        endif()

        # Content fetching
        FetchContent_Declare(${ARG_LIB_NAME}
            ${ARG_FETCH_CONTENT_ARGS}
            ${GIT_SUBMODULES}   ${GIT_SUBMODULES_VALUE}
            SOURCE_SUBDIR       non-exist-dir   # skip add_subdirectory because sources are not patched yet
        )

        FetchContent_GetProperties(${ARG_LIB_NAME})
        if(NOT ${ARG_LIB_NAME}_POPULATED)
            FetchContent_MakeAvailable(${ARG_LIB_NAME})

            # Get remote properties
            _get_fetch_remote_properties(REPO_URL_VAR TAG_VAR ${ARG_FETCH_CONTENT_ARGS})
            set(${ARG_LIB_SOURCE_PATH_VAR} ${${ARG_LIB_NAME}_SOURCE_DIR} CACHE PATH "Library has been downloaded from ${REPO_URL_VAR}, tag ${TAG_VAR}" FORCE)
            if (DEFINED ARG_LIB_BINARY_PATH_VAR)
                set(${ARG_LIB_BINARY_PATH_VAR} ${${ARG_LIB_NAME}_BINARY_DIR} CACHE PATH "Path to build directory of ${ARG_LIB_NAME}")
            endif()
        endif()
    endif()

    if (DEFINED ARG_LIB_FORCE_PATCH)
        set(FORCE_PATCH ${ARG_LIB_FORCE_PATCH})
    else()
        set(FORCE_PATCH ON)
    endif()

    if (ARG_LIB_PATCH_DIR AND (SOURCE_PATH_IS_DOWNLOAD OR FORCE_PATCH))
        # look for patch files
        file(GLOB PATCH_FILES "${ARG_LIB_PATCH_DIR}/*.patch")

        if(PATCH_FILES)
            # Apply patches for existing sources
            apply_patches("${${ARG_LIB_SOURCE_PATH_VAR}}" "${PATCH_FILES}")
        endif()
    endif()

    # Process arguments which can be an empty string
    # There is a feature/bug in CMake that result in problem with empty string arguments
    # See https://gitlab.kitware.com/cmake/cmake/-/issues/16341 for details
    extract_key_value(SOURCE_SUBDIR SOURCE_SUBDIR SOURCE_SUBDIR_VALUE ARG_FETCH_CONTENT_ARGS)
    # Add subdirectory with CMakeLists.txt in SOURCE_SUBDIR if exists
    if(EXISTS "${${ARG_LIB_SOURCE_PATH_VAR}}/${SOURCE_SUBDIR_VALUE}/CMakeLists.txt")
        if (NOT DEFINED ${ARG_LIB_NAME}_BINARY_DIR)
            # For local sources FetchContent is not used, so <name>_BINARY_DIR is
            # not defined. add_subdirectory() requires an explicit binary_dir when
            # source_dir is not under CMAKE_CURRENT_SOURCE_DIR, which is the
            # typical case for local library copies. Derive it from LIB_BASE_DIR.
            if (NOT ARG_LIB_BASE_DIR)
                message(FATAL_ERROR "fetch_remote_library(${ARG_LIB_NAME}): LIB_BASE_DIR is required for local sources to derive binary_dir for add_subdirectory()")
            endif()
            set(${ARG_LIB_NAME}_BINARY_DIR "${ARG_LIB_BASE_DIR}/${ARG_LIB_NAME}-build")
            if (DEFINED ARG_LIB_BINARY_PATH_VAR)
                set(${ARG_LIB_BINARY_PATH_VAR} ${${ARG_LIB_NAME}_BINARY_DIR} CACHE PATH "Path to build directory of ${ARG_LIB_NAME}")
            endif()
        endif()
        add_subdirectory("${${ARG_LIB_SOURCE_PATH_VAR}}" "${${ARG_LIB_NAME}_BINARY_DIR}")
    endif()
endfunction()
