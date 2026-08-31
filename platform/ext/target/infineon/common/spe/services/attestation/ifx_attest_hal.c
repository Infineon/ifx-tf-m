/*
 * (c) 2023-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "cmsis_compiler.h"
#include "fih.h"

#include "config_tfm.h"
#include "provisioning.h"
#include "tfm_attest_hal.h"
#include "tfm_plat_boot_seed.h"
#include "psa/crypto.h"

#define IFX_ATTESTATION_KEY_ID      ((psa_key_id_t)(PSA_KEY_ID_VENDOR_MIN + 1U))

#ifndef IFX_ATTESTATION_PROFILE_DEFINITION
/**
 * Default value of profile definition for Initial Attestation service.
 */
#define IFX_ATTESTATION_PROFILE_DEFINITION "tag:psacertified.org,2023:psa#tfm"
#endif

#ifndef IFX_ATTESTATION_VERIFICATION_SERVICE
/**
 * Default value of verification service for Initial Attestation.
 */
#define IFX_ATTESTATION_VERIFICATION_SERVICE "www.trustedfirmware.org"
#endif

#define BOOL_SEED_INITIALIZED       0x3Bu
#define BOOL_SEED_UNINITIALIZED     0xC4u

enum tfm_plat_err_t tfm_plat_get_boot_seed(uint32_t size, uint8_t *buf)
{
    /* \brief Is equal to \ref BOOL_SEED_INITIALIZED if boot_seed is initialized */
    static uint8_t boot_seed_initialized = BOOL_SEED_UNINITIALIZED;
    static uint8_t boot_seed[BOOT_SEED_SIZE];

    /* Warning: This validation is not compatible with default implementation.
     * Which returns up to sizeof(boot_seed) instead of returning error. */
    if ((buf == NULL) || (size != sizeof(boot_seed))) {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }

    if (boot_seed_initialized == BOOL_SEED_UNINITIALIZED) {
        if (psa_generate_random(boot_seed, sizeof(boot_seed)) == PSA_SUCCESS) {
            boot_seed_initialized = BOOL_SEED_INITIALIZED;
        }
    }

    /* Copy anyway to make time based analysis difficult */
    (void)memcpy(buf, boot_seed, size);

#ifdef TFM_FIH_PROFILE_ON
#if IFX_PSA_ROT_PRIVILEGED
    /* fih_delay uses static variables which are placed to SPM linker section,
     * so when PSA RoT is unprivileged it has no access to these variables,
     * thus it is not possible to call fih_delay or other functions that use
     * fih_delay (e.g. fih_eq, ...). */
    (void)fih_delay();
#endif /* IFX_PSA_ROT_PRIVILEGED */

    /* Fault-injection hardening: detect a skipped or partial memcpy() above.
     * __DMB() carries a "memory" clobber, so it acts as a compiler barrier as
     * well as a data memory barrier: it prevents the compiler from proving
     * that buf already equals boot_seed and folding the memcmp() away. A
     * mismatch means the copy did not take full effect, so the caller must not
     * attest a stale/zero seed. */
    __DMB();
    if (memcmp(buf, boot_seed, size) != 0) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }
#endif /* TFM_FIH_PROFILE_ON */

    return (boot_seed_initialized == BOOL_SEED_INITIALIZED) ?
            TFM_PLAT_ERR_SUCCESS : TFM_PLAT_ERR_SYSTEM_ERR;
}

enum tfm_security_lifecycle_t tfm_attest_hal_get_security_lifecycle(void)
{
    enum tfm_security_lifecycle_t lifecycle = TFM_SLC_UNKNOWN;
    uint32_t lcs = ifx_get_security_lifecycle() & PSA_LIFECYCLE_PSA_STATE_MASK;
    switch (lcs) {
        case PSA_LIFECYCLE_ASSEMBLY_AND_TEST:
            lifecycle = TFM_SLC_ASSEMBLY_AND_TEST;
            break;
        case PSA_LIFECYCLE_NON_PSA_ROT_DEBUG:
            lifecycle = TFM_SLC_NON_PSA_ROT_DEBUG;
            break;
        case PSA_LIFECYCLE_RECOVERABLE_PSA_ROT_DEBUG:
            lifecycle = TFM_SLC_RECOVERABLE_PSA_ROT_DEBUG;
            break;
        case PSA_LIFECYCLE_PSA_ROT_PROVISIONING:
            lifecycle = TFM_SLC_PSA_ROT_PROVISIONING;
            break;
        case PSA_LIFECYCLE_SECURED:
            lifecycle = TFM_SLC_SECURED;
            break;
        case PSA_LIFECYCLE_DECOMMISSIONED:
            lifecycle = TFM_SLC_DECOMMISSIONED;
            break;
        case PSA_LIFECYCLE_UNKNOWN:
        default:
            lifecycle = TFM_SLC_UNKNOWN;
            break;
    }
    return lifecycle;
}

enum tfm_plat_err_t tfm_attest_hal_get_verification_service(uint32_t *size, uint8_t *buf)
{
    static const char value[] = IFX_ATTESTATION_VERIFICATION_SERVICE;

    /* Validate parameters */
    if ((size == NULL) || (buf == NULL)) {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }

    if (*size < (sizeof(value) - 1U)) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Prepare result */
    *size = sizeof(value) - 1U;
    (void)memcpy(buf, value, *size);

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_attest_hal_get_profile_definition(uint32_t *size, uint8_t *buf)
{
    static const char value[] = IFX_ATTESTATION_PROFILE_DEFINITION;

    /* Validate parameters */
    if ((size == NULL) || (buf == NULL)) {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }

    if (*size < (sizeof(value) - 1U)) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Prepare result */
    *size = sizeof(value) - 1U;
    (void)memcpy(buf, value, *size);

    return TFM_PLAT_ERR_SUCCESS;
}
