#-------------------------------------------------------------------------------
# (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Common secure image CPU architecture configuration.

# Secure image
add_compile_definitions(DOMAIN_S=1)
add_compile_definitions(COMPONENT_SECURE_DEVICE)

set(S_BUILD ON)

if (DEFINED IFX_FAMILY_SOURCE_DIR)
    include(${IFX_FAMILY_SOURCE_DIR}/shared/cpuarch.cmake)
else()
    include(${IFX_PLATFORM_SOURCE_DIR}/shared/cpuarch.cmake)
endif()
