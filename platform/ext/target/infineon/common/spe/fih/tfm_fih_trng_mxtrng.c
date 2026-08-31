/*
 * (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "tfm_fih_trng.h"

/* IMPROVEMENT: (TFM-4742) Use a real MXTRNG-based implementation. */
#define IFX_MXTRNG_STUB_RANDOM_VALUE    (0xDEADBEEFuL)

/*
 * Obtain a 32-bit number from the TRNG
 * Returns 0 on failure.
 */
uint32_t ifx_trng(void)
{
    return (uint32_t)IFX_MXTRNG_STUB_RANDOM_VALUE;
}
