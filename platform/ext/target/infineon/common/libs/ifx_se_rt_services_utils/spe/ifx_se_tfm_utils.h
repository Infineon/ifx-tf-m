/*
 * (c) 2023-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * \file ifx_se_tfm_utils.h
 *
 * \brief This file is a wrapper for ifx-se-rt-services-utils library for TF-M.
 *        It adds additional stuff needed to use this library from within secure
 *        partitions.
 */

#ifndef IFX_SE_TFM_UTILS_H
#define IFX_SE_TFM_UTILS_H


#include <stddef.h>

#include "ifx_se_psacrypto.h"
#include "psa/error.h"

/**
 * \brief Use this macro as context parameter to ifx_se_* functions.
 *
 * Example:
 * \code
 *      ifx_se_generate_random(output, output_size, IFX_SE_TFM_SYSCALL_CONTEXT);
 * \endcode
 */
#define IFX_SE_TFM_SYSCALL_CONTEXT          NULL

/**
 * \brief Convert an ifx_se_status_t value into the corresponding psa_status_t.
 *
 * \param[in] status  Status returned by an ifx_se_* function.
 *
 * \return The PSA status that corresponds to \p status.
 */
static inline psa_status_t ifx_se_get_tfm_psa_status(ifx_se_status_t status)
{
    if (IFX_SE_IS_STATUS_SUCCESS(status)) {
        return PSA_SUCCESS;
    } else if (IFX_SE_STATUS_CHECK(status, IFX_SE_SYSCALL_NOT_SUPPORTED)) {
        return PSA_ERROR_NOT_SUPPORTED;
    } else if (IFX_SE_STATUS_CHECK(status, IFX_SE_SYSCALL_GENERAL_ERROR)) {
        return PSA_ERROR_GENERIC_ERROR;
    } else if (IFX_SE_STATUS_CHECK(status, IFX_SE_SYSCALL_INVALID_OPCODE)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    } else if (IFX_SE_STATUS_CHECK(status, IFX_SE_SYSCALL_INVALID_ARGUMENT)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    } else if (IFX_SE_STATUS_CHECK(status, IFX_SE_SYSCALL_TIMEOUT_ERROR)) {
        return PSA_ERROR_COMMUNICATION_FAILURE;
    } else if (IFX_SE_STATUS_CHECK(status, IFX_SE_SYSCALL_CORRUPTION_DETECTED)) {
        return PSA_ERROR_CORRUPTION_DETECTED;
    } else if (IFX_SE_STATUS_CHECK(status, IFX_SE_SYSCALL_INVALID_MEM_ALLOC)) {
        return PSA_ERROR_INSUFFICIENT_MEMORY;
    } else if (IFX_SE_STATUS_CHECK(status, IFX_SE_SYSCALL_STORAGE_FAILURE) ||
               IFX_SE_STATUS_CHECK(status, IFX_SE_SYSCALL_STORAGE_TIMEOUT)) {
        return PSA_ERROR_STORAGE_FAILURE;
    } else if (IFX_SE_STATUS_CHECK(status, IFX_SE_SYSCALL_BUFFER_TOO_SMALL)) {
        return PSA_ERROR_BUFFER_TOO_SMALL;
    } else if (IFX_SE_STATUS_CHECK(status, IFX_SE_SYSCALL_INSUFFICIENT_STORAGE)) {
        return PSA_ERROR_INSUFFICIENT_STORAGE;
    } else {
        return ifx_se_fih_uint_decode(status);
    }
}

#endif /* IFX_SE_TFM_UTILS_H */
