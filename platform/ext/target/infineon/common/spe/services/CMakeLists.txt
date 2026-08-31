#-------------------------------------------------------------------------------
# (c) 2023-2026, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG. All rights reserved.
# SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# SE IPC Service to handle se-rt-services-utils requests
add_subdirectory(${IFX_SE_IPC_SERVICE_PATH} se_ipc_service)

target_compile_definitions(tfm_config
    INTERFACE
        IFX_DRIVER_FLASH_PROG_BUF_ON_STACK=$<BOOL:${IFX_DRIVER_FLASH_PROG_BUF_ON_STACK}>
        IFX_DRIVER_FLASH_SINGLE_INSTANCE=$<BOOL:${IFX_DRIVER_FLASH_SINGLE_INSTANCE}>
)

################################### Drivers ####################################

target_include_directories(tfm_sprt
    PUBLIC
        ${IFX_COMMON_SOURCE_DIR}/drivers/flash/flash
        ${IFX_COMMON_SOURCE_DIR}/drivers/flash/rram
        ${IFX_COMMON_SOURCE_DIR}/drivers/flash/smif
        ${IFX_COMMON_SOURCE_DIR}/drivers/flash
)

target_sources(tfm_sprt
    PRIVATE
        $<$<BOOL:${IFX_FLASH_DRIVER_ENABLED}>:${IFX_COMMON_SOURCE_DIR}/drivers/flash/flash/ifx_driver_flash.c>
        $<$<BOOL:${IFX_RRAM_DRIVER_ENABLED}>:${IFX_COMMON_SOURCE_DIR}/drivers/flash/rram/ifx_driver_rram.c>
        $<$<OR:$<BOOL:${IFX_FLASH_DRIVER_ENABLED}>,$<BOOL:${IFX_RRAM_DRIVER_ENABLED}>>:${IFX_COMMON_SOURCE_DIR}/drivers/flash/ifx_driver_private.c>
        $<$<BOOL:${IFX_SMIF_MMIO_DRIVER_ENABLED}>:${IFX_COMMON_SOURCE_DIR}/drivers/flash/smif/ifx_driver_smif_mmio.c>
        $<$<BOOL:${IFX_SMIF_XIP_DRIVER_ENABLED}>:${IFX_COMMON_SOURCE_DIR}/drivers/flash/smif/ifx_driver_smif_xip.c>
        $<$<OR:$<BOOL:${IFX_SMIF_MMIO_DRIVER_ENABLED}>,$<BOOL:${IFX_SMIF_XIP_DRIVER_ENABLED}>>:${IFX_COMMON_SOURCE_DIR}/drivers/flash/smif/ifx_driver_smif.c>
)

################################################################################

if (TFM_PARTITION_CRYPTO)
    # Overwrite CRYPTO_ENGINE_BUF_SIZE in .h files if CRYPTO_ENGINE_BUF_SIZE is specified in CMake
    # This makes it possible to set CRYPTO_ENGINE_BUF_SIZE through CMake
    if(DEFINED CRYPTO_ENGINE_BUF_SIZE)
        target_compile_definitions(tfm_config
            INTERFACE
                CRYPTO_ENGINE_BUF_SIZE=${CRYPTO_ENGINE_BUF_SIZE}
        )
    endif()

    target_compile_definitions(platform_crypto_keys
        PRIVATE
            $<$<BOOL:${MBEDTLS_PSA_CRYPTO_PLATFORM_FILE}>:MBEDTLS_PSA_CRYPTO_PLATFORM_FILE="${MBEDTLS_PSA_CRYPTO_PLATFORM_FILE}">
    )

    if (IFX_CRYPTO_SE_RT)
        target_sources(crypto_service_tfpsacrypto
            PRIVATE
                crypto/crypto_rnd_se_rt.c
        )

        # crypto_rnd_se_rt.c selects the IFX SE-RT crypto platform definitions
        # via MBEDTLS_PSA_CRYPTO_PLATFORM_FILE; without it the file falls back to
        # a bare #include "crypto_platform.h" that is not on the core crypto
        # target's include path (only psa/crypto_platform.h is). platform_crypto_keys
        # already receives this define above; the core crypto target needs it too
        # now that it compiles SE-RT sources.
        target_compile_definitions(crypto_service_tfpsacrypto
            PRIVATE
                $<$<BOOL:${MBEDTLS_PSA_CRYPTO_PLATFORM_FILE}>:MBEDTLS_PSA_CRYPTO_PLATFORM_FILE="${MBEDTLS_PSA_CRYPTO_PLATFORM_FILE}">
        )

        target_compile_definitions(ifx_se_rt_services_utils_s
            PUBLIC
                MBEDTLS_PSA_CRYPTO_DRIVERS
        )
    endif()

    if (IFX_CRYPTO_KEYS_FLASH)
        target_sources(platform_crypto_keys
            PRIVATE
                crypto/crypto_keys_flash.c
        )

        target_link_libraries(platform_crypto_keys
            PRIVATE
                tfm_helper_lib # For ARRAY_SIZE macro
        )
    endif()

    if (IFX_CRYPTO_KEYS_RRAM)
        # psa_interface is linked to tfm_sprt, thus when tfm_sprt is linked to
        # platform_crypto_keys, types from psa_interface conflict with MbedTLS
        # native types. Because of this tfm_plat_nvm_read_asset function can not
        # be added to platform_crypto_keys using tfm_sprt, instead they have to
        # be added to platform_crypto_keys directly.
        target_include_directories(platform_crypto_keys
            PRIVATE
                ${IFX_COMMON_SOURCE_DIR}/drivers/assets
                ${IFX_COMMON_SOURCE_DIR}/drivers/flash
                ${IFX_COMMON_SOURCE_DIR}/drivers/flash/rram
        )

        target_sources(platform_crypto_keys
            PRIVATE
                crypto/crypto_keys_rram.c
                ${IFX_COMMON_SOURCE_DIR}/drivers/assets/ifx_assets_rram.c
        )

        target_link_libraries(platform_crypto_keys
            PUBLIC
                $<$<BOOL:${IFX_SE_RT_SERVICES_UTILS_ENABLED}>:ifx_se_rt_services_utils_s>
        )
    endif()

    if (IFX_CRYPTO_KEYS_SE_RT)
        target_sources(platform_crypto_keys
            PRIVATE
                crypto/crypto_keys_se_rt.c
        )

        target_link_libraries(platform_crypto_keys
            PUBLIC
                ifx_se_rt_services_utils_s
            PRIVATE
                psa_interface
        )

        target_link_libraries(crypto_service_tfpsacrypto
            PUBLIC
                ifx_se_rt_services_utils_s
        )
    endif()

    if (IFX_MBEDTLS_ACCELERATION_ENABLED)
        target_link_libraries(platform_crypto_keys
            PUBLIC
                ifx_mbedtls_acceleration
        )

        if (IFX_SE_IPC_SERVICE_TYPE STREQUAL "FULL")
            # SE RT Services Utils provides additional PSA Crypto types. mbedTLS acceleration library
            # provides PSA crypto API. Thus accelerator library is dependent on SE RT Services Utils.
            target_link_libraries(ifx_mbedtls_acceleration
                PUBLIC
                    ifx_se_rt_services_utils_s
            )
        endif()
    endif()

    target_include_directories(crypto_service_tfpsacrypto_config
        INTERFACE
            ${IFX_PLATFORM_SOURCE_DIR}/spe/services/crypto
            ${CMAKE_CURRENT_SOURCE_DIR}/crypto/mbedtls_accel_configs
            # SE RT platform support headers. Provides platform_alt.h, which
            # mbedtls/platform.h includes by name when the SE RT config defines
            # MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT.
            $<$<BOOL:${IFX_CRYPTO_SE_RT}>:${IFX_COMMON_SOURCE_DIR}/libs/tf-psa-crypto>
            )

    # The SE-DPA opaque driver-hook patch makes the TF-PSA-Crypto driver context
    # headers include SE RT Services Utils headers (ifx_se_psacrypto.h, which in
    # turn pulls in SE RT and PDL headers). Every internal crypto target (p256-m,
    # builtin, everest, ...) compiles psa/crypto.h, so they all need those headers
    # on their include path. Forward the SE RT Services Utils compile usage
    # requirements (its and PDL's include dirs and compile definitions) through
    # the shared crypto config. Use $<COMPILE_ONLY:...> so this does NOT add a
    # link/ordering dependency: ifx_se_rt_services_utils_s links back into the
    # partition graph, and a real link edge here would drag the TF-PSA-Crypto
    # OBJECT libraries into that static-library dependency cycle (CMake forbids
    # OBJECT libraries in a cyclic component).
    if (IFX_CRYPTO_SE_RT)
        target_link_libraries(crypto_service_tfpsacrypto_config
            INTERFACE
                $<COMPILE_ONLY:ifx_se_rt_services_utils_s>
        )
    endif()

    # The transparent accelerator driver-hook patches (MXCRYPTO, CRYPTOLITE,
    # CRYPTOSUITE) make the TF-PSA-Crypto driver context headers include the
    # accelerator type headers (e.g. ifx_mxcrypto_transparent_types.h, which in
    # turn pulls in accelerator and PDL headers) when IFX_PSA_*_PRESENT is set.
    # Those PRESENT macros are only defined when IFX_MBEDTLS_ACCELERATION_ENABLED,
    # which is also when the ifx_mbedtls_acceleration target exists. Forward its
    # compile usage requirements (its own and, transitively via its PUBLIC PDL
    # link, the PDL include dirs and device-selection compile definitions) to
    # every internal crypto target through the shared crypto config. As above use
    # $<COMPILE_ONLY:...> so no link/ordering dependency is added (which would
    # recreate the static-library cycle that traps the TF-PSA-Crypto OBJECT
    # libraries).
    if (IFX_MBEDTLS_ACCELERATION_ENABLED)
        target_link_libraries(crypto_service_tfpsacrypto_config
            INTERFACE
                $<COMPILE_ONLY:ifx_mbedtls_acceleration>
        )
    endif()

    # The accelerator driver-hook patches add conditional union members (e.g.
    # ifx_cryptolite_/ifx_cryptosuite_/ifx_mxcrypto_*_operation_t) to the
    # psa_driver_*_context_t structs in the patched copy of crypto_driver_contexts_*.h
    # under ${TF_PSA_CRYPTO_PATH}/include (patches are applied in-place at that path by
    # apply_patches() in lib/ext/tf-psa-crypto/CMakeLists.txt).
    # The crypto partition's default include order resolves those headers from the
    # un-patched interface/include copy FIRST, so the partition would compile the
    # context structs WITHOUT the accelerator members while the crypto service
    # libraries (which see the patched copy first) compile them WITH the members.
    # That size/layout disagreement is an ODR violation: the build succeeds but the
    # accelerator contexts land at the wrong offsets at runtime, so the hardware
    # crypto silently never engages. Put the patched include dir first for the
    # partition so its struct layout matches the crypto libraries.
    # NOTE: use ${TF_PSA_CRYPTO_PATH} (the actual patched source location) rather than a
    # hardcoded ${CMAKE_BINARY_DIR}/lib/ext/tf-psa-crypto-src path. In a native/CI build
    # TF_PSA_CRYPTO_PATH resolves to that same fetched dir, but when TF-PSA-Crypto is
    # supplied as an external prebuilt tree (e.g. ModusToolbox mtb_shared) the -src dir
    # does not exist and the hardcoded path silently no-ops, re-introducing the ODR crash.
    if (IFX_MBEDTLS_ACCELERATION_ENABLED AND TARGET tfm_psa_rot_partition_crypto)
        target_include_directories(tfm_psa_rot_partition_crypto
            BEFORE PRIVATE
                ${TF_PSA_CRYPTO_PATH}/include
        )
    endif()

    target_compile_definitions(crypto_service_tfpsacrypto_config
        INTERFACE
                        $<$<BOOL:${CRYPTO_HW_ACCELERATOR_CONFIG}>:CRYPTO_HW_ACCELERATOR_CONFIG="${CRYPTO_HW_ACCELERATOR_CONFIG}">
            $<$<BOOL:${IFX_MBEDTLS_CONFIG_PATH}>:IFX_MBEDTLS_CONFIG_PATH="${IFX_MBEDTLS_CONFIG_PATH}">
    )

    target_compile_definitions(psa_crypto_config
        INTERFACE
            $<BUILD_INTERFACE:$<$<BOOL:${CRYPTO_HW_ACCELERATOR_CONFIG}>:CRYPTO_HW_ACCELERATOR_CONFIG="${CRYPTO_HW_ACCELERATOR_CONFIG}">>
    )

    if (NOT PLATFORM_DEFAULT_NV_SEED)
        target_sources(platform_s
            PRIVATE
                # Common nv seed functionality
                ${IFX_COMMON_SOURCE_DIR}/spe/services/crypto/crypto_nv_seed.c

                # NV seed driver depending on acceleration type. Used regardless
                # of IFX_MBEDTLS_ACCELERATION_ENABLED value.
                $<$<STREQUAL:${IFX_MBEDTLS_ACCELERATOR_TYPE},MXCRYPTO>:${IFX_COMMON_SOURCE_DIR}/spe/services/crypto/crypto_nv_seed_mxcrypto.c>
                $<$<STREQUAL:${IFX_MBEDTLS_ACCELERATOR_TYPE},CRYPTOLITE>:${IFX_COMMON_SOURCE_DIR}/spe/services/crypto/crypto_nv_seed_cryptolite.c>
                $<$<STREQUAL:${IFX_MBEDTLS_ACCELERATOR_TYPE},MXTRNG>:${IFX_COMMON_SOURCE_DIR}/spe/services/crypto/crypto_nv_seed_mxtrng.c>
        )
    endif()
endif()

if (TFM_PARTITION_INITIAL_ATTESTATION)
    target_sources(tfm_psa_rot_partition_attestation
        PRIVATE
            attestation/ifx_attest_hal.c
            $<$<BOOL:${IFX_PLATFORM_DEFAULT_DEVICE_ID}>:${CMAKE_CURRENT_SOURCE_DIR}/attestation/ifx_plat_device_id.c>
            $<$<BOOL:${IFX_INITIAL_ATTESTATION_SE_RT}>:${CMAKE_CURRENT_SOURCE_DIR}/attestation/ifx_attest_hal_se_rt.c>
    )

    target_compile_definitions(tfm_psa_rot_partition_attestation
        PRIVATE
            PLATFORM_CREATES_ATTEST_TOKEN=$<BOOL:${IFX_INITIAL_ATTESTATION_SE_RT}>
            IFX_INITIAL_ATTESTATION_SE_RT=$<BOOL:${IFX_INITIAL_ATTESTATION_SE_RT}>
    )

    target_include_directories(tfm_psa_rot_partition_attestation
        PRIVATE
            ${IFX_COMMON_SOURCE_DIR}/spe/provisioning
            ${IFX_COMMON_SOURCE_DIR}/drivers/assets # For ifx_assets_rram.h
    )

    target_link_libraries(tfm_psa_rot_partition_attestation
        PRIVATE
            $<$<BOOL:${IFX_SE_RT_SERVICES_UTILS_ENABLED}>:ifx_se_rt_services_utils_s>
    )
endif()

if (TFM_PARTITION_INTERNAL_TRUSTED_STORAGE)
    target_sources(tfm_psa_rot_partition_its
        PRIVATE
            "$<$<IN_LIST:FLASH,${IFX_ITS_VALID_TYPES}>:${IFX_COMMON_SOURCE_DIR}/spe/services/its/drivers/its_flash_driver.c>"
            "$<$<IN_LIST:RRAM,${IFX_ITS_VALID_TYPES}>:${IFX_COMMON_SOURCE_DIR}/spe/services/its/drivers/its_rram_driver.c>"
            ${IFX_COMMON_SOURCE_DIR}/spe/services/its/its_hal.c
    )

    target_include_directories(tfm_psa_rot_partition_its
        PRIVATE
            ${IFX_COMMON_SOURCE_DIR}/drivers/flash/rram
            ${IFX_COMMON_SOURCE_DIR}/drivers/flash/smif
            ${IFX_COMMON_SOURCE_DIR}/drivers/flash
            ${IFX_COMMON_SOURCE_DIR}/spe/services/its
    )
endif()

if (TFM_PARTITION_NS_AGENT_MAILBOX)
    target_sources(tfm_psa_rot_partition_ns_agent_mailbox
        PRIVATE
            mailbox/platform_spe_mailbox.c
            mailbox/tfm_hal_multi_core.c
            mailbox/tfm_interrupts.c
            ${IFX_COMMON_SOURCE_DIR}/shared/mailbox/platform_multicore.c
    )

    target_include_directories(tfm_psa_rot_partition_ns_agent_mailbox
        PRIVATE
            mailbox
            ${IFX_COMMON_SOURCE_DIR}/shared/mailbox/
            ${IFX_GENERATED_DIR}/interface/include
            ${CMAKE_SOURCE_DIR}/interface/include/multi_core
    )

    target_sources(tfm_psa_rot_partition_ns_agent_mailbox
        PRIVATE
            $<$<BOOL:${IFX_MULTICORE_CM55}>:${CMAKE_CURRENT_LIST_DIR}/mailbox/platform_hal_multi_core_cm55.c>
    )
endif()

if (TFM_PARTITION_PLATFORM)
    target_sources(tfm_psa_rot_partition_platform
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/platform/nv_counters_common.c
            "$<$<IN_LIST:SE_RT,${IFX_NV_COUNTERS_VALID_TYPES}>:${CMAKE_CURRENT_SOURCE_DIR}/platform/nv_counters_se_rt.c>"
            "$<$<IN_LIST:FLASH,${IFX_NV_COUNTERS_VALID_TYPES}>:${CMAKE_CURRENT_SOURCE_DIR}/platform/nv_counters_flash.c>"
            "$<$<IN_LIST:FLASH,${IFX_NV_COUNTERS_VALID_TYPES}>:${CMAKE_CURRENT_SOURCE_DIR}/platform/drivers/nv_counters_flash_driver.c>"
            "$<$<IN_LIST:RRAM,${IFX_NV_COUNTERS_VALID_TYPES}>:${CMAKE_CURRENT_SOURCE_DIR}/platform/nv_counters_rram.c>"
            "$<$<IN_LIST:RRAM,${IFX_NV_COUNTERS_VALID_TYPES}>:${CMAKE_CURRENT_SOURCE_DIR}/platform/drivers/nv_counters_rram_driver.c>"
            $<$<NOT:$<BOOL:${IFX_SE_RT_SERVICES_UTILS_ENABLED}>>:${IFX_COMMON_SOURCE_DIR}/utils/se/ifx_se_crc32.c>

            platform/tfm_platform_system.c
    )

    target_include_directories(tfm_psa_rot_partition_platform
        PRIVATE
            platform
            platform/drivers
            ${IFX_COMMON_SOURCE_DIR}/interface/include
            ${IFX_COMMON_SOURCE_DIR}/interface/src
            ${IFX_COMMON_SOURCE_DIR}/drivers/flash/rram
            ${IFX_COMMON_SOURCE_DIR}/drivers/flash
            $<$<NOT:$<BOOL:${IFX_SE_RT_SERVICES_UTILS_ENABLED}>>:${IFX_COMMON_SOURCE_DIR}/utils/se>
    )

    target_link_libraries(tfm_psa_rot_partition_platform
        PRIVATE
            $<$<BOOL:${IFX_SE_RT_SERVICES_UTILS_ENABLED}>:ifx_se_rt_services_utils_s>
            tfm_spm
    )
endif()

if (TFM_PARTITION_PROTECTED_STORAGE)
    target_sources(tfm_app_rot_partition_ps
        PRIVATE
            $<$<AND:$<BOOL:${IFX_CRYPTO_SE_RT}>,$<BOOL:${IFX_PARTITION_SE_IPC_SERVICE}>>:${IFX_COMMON_SOURCE_DIR}/spe/services/ps/ps_keys_se_rt.c>
    )

    target_sources(tfm_psa_rot_partition_its
        PRIVATE
            ${IFX_COMMON_SOURCE_DIR}/spe/services/ps/ps_hal.c
            "$<$<IN_LIST:SMIF_MMIO,${IFX_PS_VALID_TYPES}>:${IFX_COMMON_SOURCE_DIR}/spe/services/ps/drivers/ps_smif_driver.c>"
            "$<$<IN_LIST:RRAM,${IFX_PS_VALID_TYPES}>:${IFX_COMMON_SOURCE_DIR}/spe/services/ps/drivers/ps_rram_driver.c>"
            "$<$<IN_LIST:FLASH,${IFX_PS_VALID_TYPES}>:${IFX_COMMON_SOURCE_DIR}/spe/services/ps/drivers/ps_flash_driver.c>"
    )
endif()

################################# TF-M Tests ##################################

if (TFM_S_REG_TEST OR TFM_NS_REG_TEST)
    target_sources(platform_s
        PRIVATE
            tfm_tests/plat_test.c
    )
endif()

if(TEST_NS_SLIH_IRQ)
    target_sources(tfm_app_rot_partition_slih_test
        PRIVATE
            tfm_tests/plat_test_secure_timer.c
    )
elseif(TEST_NS_FLIH_IRQ)
    target_sources(tfm_app_rot_partition_flih_test
        PRIVATE
            tfm_tests/plat_test_secure_timer.c
    )
endif()

if (TEST_PSA_API STREQUAL IPC)
    target_sources(platform_s
        PRIVATE
            tfm_tests/plat_test_secure_timer.c
            tfm_tests/plat_test.c
    )
endif()

if (TEST_NS_FLIH_IRQ OR TEST_NS_SLIH_IRQ OR (TEST_PSA_API STREQUAL IPC))
    target_sources(tfm_spm
        PRIVATE
            tfm_tests/tfm_interrupts_test_s.c
    )
endif()
