/*
 * (c) 2023-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "cy_pdl.h"
#include "ifx_assets_rram.h"
#include "ifx_driver_rram.h"
#include "ifx_se_crc32.h"
#include "coverity_check.h"


int32_t ifx_assets_rram_read_block(uint32_t address, void *data, uint32_t length)
{
    cy_en_rram_status_t rram_status = CY_RRAM_BAD_PARAM;
    int32_t err = ARM_DRIVER_ERROR_PARAMETER;
    int32_t retries = IFX_DRIVER_RRAM_PC_LOCK_RETRIES;

    do {
        rram_status = Cy_RRAM_TSReadByteArray(IFX_RRAMC0, address, data, length);

        if (rram_status != CY_RRAM_ACQUIRE_PC_LOCK_FAIL) {
            break;
        }
    } while (--retries > 0);

    if (rram_status == CY_RRAM_SUCCESS) {
        err = ARM_DRIVER_OK;
    } else if (rram_status == CY_RRAM_ACQUIRE_PC_LOCK_FAIL) {
        err = ARM_DRIVER_ERROR_TIMEOUT;
    } else {
        err = ARM_DRIVER_ERROR;
    }

    return err;
}

int32_t ifx_assets_rram_read_asset(uint32_t address, void *asset, size_t size)
{
    int32_t err = ARM_DRIVER_ERROR_PARAMETER;
    uint32_t checksum = 0u;

    err = ifx_assets_rram_read_block(address, asset, size);
    if (err != ARM_DRIVER_OK) {
        return err;
    }

    err = ifx_assets_rram_read_block((address + size), (uint8_t *) &checksum, sizeof(checksum));
    if (err != ARM_DRIVER_OK) {
        return err;
    }

    /* Redundant, cross-checked comparison so a single fault on the acceptance
     * decision cannot pass a corrupted asset. The comparisons are kept in
     * separate if statements so the compiler cannot fold them into a single
     * branch that one glitch could skip. */
    volatile uint32_t crc = ifx_se_crc32d6a(size, asset, IFX_S_ADDRESS_ALIAS(address));
    volatile uint32_t crc_verify = ifx_se_crc32d6a(size, asset, IFX_S_ADDRESS_ALIAS(address));
    if (checksum != crc) {
        return ARM_DRIVER_ERROR;
    }
    if (checksum != crc_verify) {
        return ARM_DRIVER_ERROR;
    }
    TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Rule_13_2, "Redundant volatile reads are intentional for fault detection; evaluation order does not matter")
    TFM_COVERITY_DEVIATE_LINE(cert_exp30_c, "Volatile variables do not have side-effects, so read order doesn't matter")
    if (crc != crc_verify) {
        return ARM_DRIVER_ERROR;
    }
    TFM_COVERITY_BLOCK_END(MISRA_C_2023_Rule_13_2)

    return ARM_DRIVER_OK;
}
