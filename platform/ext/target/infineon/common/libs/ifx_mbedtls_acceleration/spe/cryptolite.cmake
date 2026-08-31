#-------------------------------------------------------------------------------
# (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Set default path for Cryptolite PSA
set(IFX_CRYPTOLITE_PSA_PATH "mbedtls_psa_MXCryptolite" CACHE STRING "Path to Cryptolite PSA implementation")

# Handle legacy file structure
if(IFX_DEVICE_CATEGORY MATCHES "CAT1")
    set(IFX_CRYPTOLITE_COMPONENT_PATH "/COMPONENT_CAT1" CACHE STRING "Cryptolite component subdirectory")
endif()

target_sources(ifx_mbedtls_acceleration
    PRIVATE
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/mbedtls_MXCRYPTOLITE/ecdsa_alt_mxcryptolite.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/mbedtls_MXCRYPTOLITE/sha256_alt_mxcryptolite.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/mbedtls_MXCRYPTOLITE/trng_alt_mxcryptolite.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/${IFX_CRYPTOLITE_PSA_PATH}/ifx_cryptolite_common.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/${IFX_CRYPTOLITE_PSA_PATH}/ifx_cryptolite_transparent_aead.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/${IFX_CRYPTOLITE_PSA_PATH}/ifx_cryptolite_transparent_cipher.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/${IFX_CRYPTOLITE_PSA_PATH}/ifx_cryptolite_transparent_hash.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/${IFX_CRYPTOLITE_PSA_PATH}/ifx_cryptolite_transparent_key_agreement.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/${IFX_CRYPTOLITE_PSA_PATH}/ifx_cryptolite_transparent_key_generation.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/${IFX_CRYPTOLITE_PSA_PATH}/ifx_cryptolite_transparent_mac.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/${IFX_CRYPTOLITE_PSA_PATH}/ifx_cryptolite_transparent_public_key_export.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/${IFX_CRYPTOLITE_PSA_PATH}/ifx_cryptolite_transparent_sign.c
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/${IFX_CRYPTOLITE_PSA_PATH}/ifx_cryptolite_transparent_signature_verify.c
)

target_include_directories(ifx_mbedtls_acceleration
    PUBLIC
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/${IFX_CRYPTOLITE_PSA_PATH}
        ${IFX_MBEDTLS_ACCELERATION_LIB_PATH}${IFX_CRYPTOLITE_COMPONENT_PATH}/mbedtls_MXCRYPTOLITE
)
