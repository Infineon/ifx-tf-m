#-------------------------------------------------------------------------------
# (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#################################### IFX #######################################

set(IFX_MTB_BUILD             OFF         CACHE BOOL "Enable build of artifacts used by ModusToolBox")

if(IFX_MTB_BUILD)
    # Sources generation is only needed for stand alone build. In MTB build, sources
    # generation is handled by MTB.
    set(IFX_GENERATE_BSP_SOURCES    OFF CACHE BOOL "Whether to generate BSP sources")
endif()

set(IFX_GENERATE_BSP_SOURCES    ON  CACHE BOOL "Whether to generate BSP sources")

################################# Dependencies #################################

set(IFX_LIB_BASE_DIR          "${CMAKE_BINARY_DIR}/lib/ext" CACHE PATH "Path to folder where libraries are downloaded to speed up build process by re-using sources")

set(IFX_CORE_LIB_PATH         "DOWNLOAD"  CACHE PATH "Path to Infineon Core library (or DOWNLOAD to fetch automatically)")
set(IFX_CORE_LIB_GIT_REMOTE   "https://github.com/Infineon/core-lib.git" CACHE STRING "Infineon Core library repo URL")
set(IFX_CORE_LIB_VERSION      "release-v1.7.0" CACHE STRING "The version of Infineon Core library to use")

set(IFX_MTB_SCMI_LIB_PATH       "DOWNLOAD"  CACHE PATH "Path to Infineon MTB SCMI library (or DOWNLOAD to fetch automatically)")
set(IFX_MTB_SCMI_LIB_GIT_REMOTE "https://github.com/Infineon/mtb-scmi.git" CACHE STRING "Infineon MTB SCMI library repo URL")
set(IFX_MTB_SCMI_LIB_VERSION    "release-v1.0.0" CACHE STRING "The version of Infineon MTB SCMI library to use")

set(TFM_EXTRAS_GIT_REMOTE     "https://github.com/Infineon/ifx-tf-m-extras.git" CACHE STRING "The URL to retrieve tf-m-extras from")
# IMPROVEMENT: Use release tag
set(TFM_EXTRAS_REPO_VERSION   "develop" CACHE STRING "The version of the ifx-tf-m-extras repo to use")
