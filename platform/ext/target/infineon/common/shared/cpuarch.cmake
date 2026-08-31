#-------------------------------------------------------------------------------
# (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Common CPU architecture configuration for Infineon platforms.

include(${CMAKE_CURRENT_LIST_DIR}/core_defs.cmake)

if (NOT DEFINED IFX_CORE)
    message(FATAL_ERROR "IFX_CORE must be defined")
endif()

# TFM is built without BL2 but external build of bootloader is present on
# device, this bootloader will populate shared data, so BOOT_DATA_AVAILABLE
# needs to be defined.
add_compile_definitions(BOOT_DATA_AVAILABLE)

# Derive CPU-specific settings from IFX_CORE
if (IFX_CORE STREQUAL ${IFX_CM33})
    # CM33 supports the single-precision FPv5-SP-D16 variant.
    set(CONFIG_TFM_FP_ARCH     "fpv5-sp-d16" CACHE STRING "FPU architecture")
    set(CONFIG_TFM_FP_ARCH_ASM "FPv5-SP" CACHE STRING "FPU architecture for assembly")
    set(TFM_SYSTEM_PROCESSOR    cortex-m33)
    set(TFM_SYSTEM_ARCHITECTURE armv8-m.main)
    set(TFM_SYSTEM_DSP          OFF   CACHE BOOL "Whether the CPU has DSP support")
    add_compile_definitions(IFX_CORE=IFX_CM33)
elseif (IFX_CORE STREQUAL ${IFX_CM55})
    # CM55 Armclang objects are tagged as FPv5-D16 by default.
    set(CONFIG_TFM_FP_ARCH     "fpv5-d16" CACHE STRING "FPU architecture")
    set(CONFIG_TFM_FP_ARCH_ASM "FPv5_D16" CACHE STRING "FPU architecture for assembly")
    set(TFM_SYSTEM_PROCESSOR    cortex-m55)
    set(TFM_SYSTEM_ARCHITECTURE armv8.1-m.main)
    set(TFM_SYSTEM_DSP          ON    CACHE BOOL "Whether the CPU has DSP support")
    add_compile_definitions(IFX_CORE=IFX_CM55)
else()
    message(FATAL_ERROR "Unsupported IFX_CORE value: ${IFX_CORE}")
endif()
