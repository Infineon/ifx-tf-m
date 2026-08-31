/*
 * (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "protection_shared_data.h"

volatile uint32_t ifx_spm_state     = IFX_SPM_STATE_INITIALIZING;
volatile uint32_t ifx_spm_state_inv = ~(uint32_t)IFX_SPM_STATE_INITIALIZING;
