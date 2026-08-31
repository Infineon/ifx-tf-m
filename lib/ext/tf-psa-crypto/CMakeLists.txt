#-------------------------------------------------------------------------------
# SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

fetch_remote_library(
    LIB_NAME                tf-psa-crypto
    LIB_SOURCE_PATH_VAR     TF_PSA_CRYPTO_PATH
    LIB_PATCH_DIR           ${CMAKE_CURRENT_LIST_DIR}
    LIB_FORCE_PATCH         ${TF_PSA_CRYPTO_FORCE_PATCH}
    FETCH_CONTENT_ARGS
        GIT_REPOSITORY      ${TF_PSA_CRYPTO_GIT_REMOTE}
        GIT_TAG             ${TF_PSA_CRYPTO_VERSION}
        GIT_SHALLOW         TRUE
        GIT_PROGRESS        TRUE
# The FetchContent_MakeAvailable() function automatically adds the fetched
# library directory to the build using add_subdirectory(). However, the TF-M
# does not need the whole TF-PSA-Crypto remote library so it defines its own
# custom targets. We set the SOURCE_SUBDIR variable to a non-existent directory
# as a workaround.
# Related details: https://gitlab.kitware.com/cmake/cmake/-/issues/26220
        SOURCE_SUBDIR       non-exist-dir
)

################################################################################
# Apply platform-specific tf-psa-crypto patches (if provided via patch.cmake)
################################################################################

if(DEFINED PLATFORM_TF_PSA_CRYPTO_PATCH_DIR AND EXISTS ${PLATFORM_TF_PSA_CRYPTO_PATCH_DIR})
    file(GLOB PLATFORM_PATCHES
         LIST_DIRECTORIES false
         ${PLATFORM_TF_PSA_CRYPTO_PATCH_DIR}/*.patch)

    if(PLATFORM_PATCHES)
        message(STATUS "Applying platform-specific tf-psa-crypto patches from: ${PLATFORM_TF_PSA_CRYPTO_PATCH_DIR}")
        apply_patches("${TF_PSA_CRYPTO_PATH}" "${PLATFORM_PATCHES}")
        message(STATUS "Platform tf-psa-crypto patches applied successfully")
    else()
        message(STATUS "No platform-specific tf-psa-crypto patches found in: ${PLATFORM_TF_PSA_CRYPTO_PATCH_DIR}")
    endif()
endif()
