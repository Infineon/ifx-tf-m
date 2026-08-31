/*
 * (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef IFX_FIH_H
#define IFX_FIH_H

#include "aapcs_local.h"
#include "fih.h"

#ifdef FIH_ENABLE_DOUBLE_VARS

#define IFX_FIH_TRUE                        ((fih_int)FIH_INT_INIT(FIH_POSITIVE_VALUE))
#define IFX_FIH_FALSE                       ((fih_int)FIH_INT_INIT(FIH_NEGATIVE_VALUE))
#define IFX_FIH_BOOL                        fih_int
#define FIH_INVALID_VALUE                   { .val = FIH_POSITIVE_VALUE, .msk = FIH_NEGATIVE_VALUE }

/*
 * Type used to return fih_int to assembler code via registers for AAPCS ABI.
 * AAPCS_DUAL_U32_T is not working with gcc 11.3 when there are three arguments to function.
 */
typedef uint64_t ifx_aapcs_fih_int;

/* Converts fih_int to ifx_aapcs_fih_int */
static __attribute__((always_inline)) inline
uint64_t ifx_fih_to_aapcs_fih(fih_int x)
{
    AAPCS_DUAL_U32_T ret;
    AAPCS_DUAL_U32_SET(ret, x.val, x.msk);
    return ret.u64_val;
}

/* IMPROVEMENT: (TFM-4579) Ideally common FIH macros should be used instead of IFX ones */
#define IFX_FIH_EQ(x, y)                                                   \
        ( ((x).val == (y).val) &&                                          \
          (bool)fih_delay() &&                                             \
          ((y).val == (int)(((unsigned)(x).msk) ^ (unsigned)_fih_mask)) && \
          (bool)fih_delay() &&                                             \
          (fih_int_validate(x), true) &&                                   \
          (((x).msk) == ((y).msk))                                         \
        )

#else /* FIH_ENABLE_DOUBLE_VARS */

#define IFX_FIH_TRUE                        true
#define IFX_FIH_FALSE                       false
#define IFX_FIH_BOOL                        bool
#define FIH_INVALID_VALUE                   (-1)

/*! Type used to return uint32_t to assembler code via registers for AAPCS ABI */
typedef uint32_t ifx_aapcs_fih_int;

/* Converts fih_int to ifx_aapcs_fih_int */
#define ifx_fih_to_aapcs_fih(x)             (x)

#define IFX_FIH_EQ(x, y)                    ((x) == (y))

#endif /* FIH_ENABLE_DOUBLE_VARS */

#define IFX_FIH_DECLARE(type, var, val) \
    FIH_RET_TYPE(type) FIH_SET(var, (FIH_RET_TYPE(type))(val))

#ifdef TFM_FIH_PROFILE_ON
/*
 * Update a volatile register with double check when FIH is enabled
 *
 * \param reg[in/out]   The volatile register
 * \param set_mask[in]  This bits will be set in the volatile register
 * \param clr_mask[in]  This bits will be cleared in the volatile register
 */
#define FIH_WRITE_REG(reg, set_mask, clr_mask) \
    do { \
        reg = ((reg) & ~(clr_mask)) | (set_mask); \
        (void)fih_delay(); \
        if (((reg) & ((set_mask) | (clr_mask))) != (set_mask)) {\
            FIH_PANIC; \
        } \
    } while (0)

#else /* TFM_FIH_PROFILE_ON */

/*
 * Update a volatile register with double check when FIH is enabled
 *
 * \param reg[in/out]   The volatile register
 * \param set_mask[in]  This bits will be set in the volatile register
 * \param clr_mask[in]  This bits will be cleared in the volatile register
 */
#define FIH_WRITE_REG(reg, set_mask, clr_mask) \
    reg = ((reg) & ~((set_mask) | (clr_mask))) | (set_mask)

#endif /* TFM_FIH_PROFILE_ON */

/*
 * Set bits in a volatile register with double check when FIH is enabled
 *
 * \param reg[in/out]   The volatile register
 * \param mask[in]      This bits will be set in the volatile register
 */
#define FIH_SET_REG(reg, mask)      FIH_WRITE_REG(reg, mask, 0)

/*
 * Clear bits in a volatile register with double check when FIH is enabled
 *
 * \param reg[in/out]   The volatile register
 * \param mask[in]      This bits will be cleared in the volatile register
 */
#define FIH_CLEAR_REG(reg, mask)    FIH_WRITE_REG(reg, 0, mask)

#endif /* IFX_FIH_H */
