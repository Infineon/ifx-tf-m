#-------------------------------------------------------------------------------
# (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

fetch_remote_library(
    LIB_NAME                ifx_mtb_scmi
    LIB_SOURCE_PATH_VAR     IFX_MTB_SCMI_LIB_PATH
    LIB_BASE_DIR            "${IFX_LIB_BASE_DIR}"
    LIB_PATCH_DIR           ${IFX_MTB_SCMI_LIB_PATCH_DIR}
    LIB_FORCE_PATCH         ON
    FETCH_CONTENT_ARGS
        GIT_REPOSITORY      ${IFX_MTB_SCMI_LIB_GIT_REMOTE}
        GIT_TAG             ${IFX_MTB_SCMI_LIB_VERSION}
        GIT_SHALLOW         TRUE
        GIT_PROGRESS        TRUE
        GIT_SUBMODULES      ""
)

add_library(ifx_mtb_scmi_int INTERFACE)

target_include_directories(ifx_mtb_scmi_int
    INTERFACE
        ${IFX_MTB_SCMI_LIB_PATH}/include
)

target_compile_definitions(ifx_mtb_scmi_int
    INTERFACE
        COMPONENT_MW_MTB_SCMI
)

target_link_libraries(ifx_mtb_scmi_int
    INTERFACE
        ifx_mtb_srf_int
)

if (S_BUILD)
    add_library(ifx_mtb_scmi_s)

    target_sources(ifx_mtb_scmi_s
        PRIVATE
            ${IFX_MTB_SCMI_LIB_PATH}/source/COMPONENT_SECURE_DEVICE/mtb_scmi_base.c
            ${IFX_MTB_SCMI_LIB_PATH}/source/COMPONENT_SECURE_DEVICE/mtb_scmi_clock.c
            ${IFX_MTB_SCMI_LIB_PATH}/source/COMPONENT_SECURE_DEVICE/mtb_scmi_notify_s.c
            ${IFX_MTB_SCMI_LIB_PATH}/source/COMPONENT_SECURE_DEVICE/mtb_scmi_power.c
            ${IFX_MTB_SCMI_LIB_PATH}/source/COMPONENT_SECURE_DEVICE/mtb_scmi_syspower.c
            ${IFX_MTB_SCMI_LIB_PATH}/source/COMPONENT_SECURE_DEVICE/mtb_scmi.c
            ${IFX_MTB_SCMI_LIB_PATH}/source/mtb_scmi_bsp_config_instances.c
    )

    target_include_directories(ifx_mtb_scmi_s
        PUBLIC
            ${IFX_MTB_SCMI_LIB_PATH}/include/COMPONENT_SECURE_DEVICE
    )

    target_link_libraries(ifx_mtb_scmi_s
        PRIVATE
            cmsis
            ifx_core_lib
            ifx_mtb_srf_s
            ifx_pdl_inc_s
        PUBLIC
            ifx_mtb_scmi_int
    )

    target_compile_options(ifx_mtb_scmi_s
        PRIVATE
            ${COMPILER_CP_FLAG}
    )

    target_link_libraries(ifx_pdl_inc_s
        INTERFACE
            ifx_mtb_scmi_int
    )
endif()

if (NS_BUILD)
    add_library(ifx_mtb_scmi_ns)

    target_sources(ifx_mtb_scmi_ns
        PRIVATE
            ${IFX_MTB_SCMI_LIB_PATH}/source/COMPONENT_NON_SECURE_DEVICE/mtb_scmi_agent.c
            ${IFX_MTB_SCMI_LIB_PATH}/source/COMPONENT_NON_SECURE_DEVICE/mtb_scmi_ipc_backend.c
            ${IFX_MTB_SCMI_LIB_PATH}/source/COMPONENT_NON_SECURE_DEVICE/mtb_scmi_notify.c
    )

    target_include_directories(ifx_mtb_scmi_ns
        PUBLIC
            ${IFX_MTB_SCMI_LIB_PATH}/include/COMPONENT_NON_SECURE_DEVICE
    )

    target_link_libraries(ifx_mtb_scmi_ns
        PRIVATE
            ifx_cmsis_lib
            ifx_core_lib
            ifx_mtb_srf_ns
            ifx_pdl_inc_ns
        PUBLIC
            ifx_mtb_scmi_int
    )

    target_link_libraries(tfm_api_ns
        PUBLIC
            ifx_mtb_scmi_ns
    )

    target_link_libraries(ifx_pdl_inc_ns
        INTERFACE
            ifx_mtb_scmi_int
    )
endif()
