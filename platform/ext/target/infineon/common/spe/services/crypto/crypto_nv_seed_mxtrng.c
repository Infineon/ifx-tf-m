/*
 * (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stddef.h>
#include <stdint.h>

#include "coverity_check.h"
#include "tfm_plat_crypto_nv_seed.h"

/* IMPROVEMENT: (TFM-4742) Use a real MXTRNG-based implementation. */
TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Directive_4_6, "Writing functions as in ARM templates, so we keep numerical types")
int tfm_plat_crypto_nv_seed_read(unsigned char *buf, size_t buf_len)
{
    static const uint8_t stub_seed[] = {
        0x4dU, 0x58U, 0x54U, 0x52U, 0x4eU, 0x47U, 0x53U, 0x54U, /* "MXTRNGST" */
        0x55U, 0x42U, 0x53U, 0x45U, 0x45U, 0x44U, 0x21U, 0x21U, /* "UBSEED!!" */
    };

    for (size_t idx = 0U; idx < buf_len; idx++) {
        buf[idx] = stub_seed[idx % sizeof(stub_seed)];
    }

    return TFM_CRYPTO_NV_SEED_SUCCESS;
}
TFM_COVERITY_BLOCK_END(MISRA_C_2023_Directive_4_6)
