#-------------------------------------------------------------------------------
# SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
# (c) 2022-2026, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

fetch_remote_library(
    LIB_NAME                tf-m-extras
    LIB_SOURCE_PATH_VAR     TFM_EXTRAS_REPO_PATH
    LIB_BASE_DIR            ${CMAKE_BINARY_DIR}/lib/ext
    FETCH_CONTENT_ARGS
        GIT_REPOSITORY      ${TFM_EXTRAS_GIT_REMOTE}
        GIT_TAG             ${TFM_EXTRAS_REPO_VERSION}
        GIT_SHALLOW         FALSE
        GIT_PROGRESS        TRUE
)
