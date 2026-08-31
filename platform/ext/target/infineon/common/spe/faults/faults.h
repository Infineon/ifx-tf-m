/*
 * (c) 2023-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef FAULTS_H
#define FAULTS_H

#include "fih.h"
#include "tfm_plat_defs.h"

/**
 * \brief Configures fault handlers and sets priorities.
 *
 * \return Returns values as specified by the \ref tfm_plat_err_t
 */
FIH_RET_TYPE(enum tfm_plat_err_t) ifx_faults_cfg(void);

/**
 * \brief This function enables the faults interrupts
 *        (plus the isolation boundary violation interrupts)
 *
 * \return Returns values as specified by the \ref tfm_plat_err_t
 */
FIH_RET_TYPE(enum tfm_plat_err_t) ifx_faults_interrupt_enable(void);

/**
 * \brief This function enables platform specific faults interrupts
 *
 * \return Returns values as specified by the \ref tfm_plat_err_t
 */
FIH_RET_TYPE(enum tfm_plat_err_t) ifx_faults_platform_interrupt_enable(void);

#if IFX_FAULTS_INFO_DUMP
/**
 * \brief Dumps platform specific information captured by fault peripherals.
 */
void ifx_faults_dump(void);
#endif

#endif /* FAULTS_H */
