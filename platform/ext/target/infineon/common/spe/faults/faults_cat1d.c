/*
 * (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "config_tfm.h"
#include "cmsis.h"
#include "cy_sysfault.h"
#include "faults.h"
#include "faults_dump.h"
#include "ifx_utils.h"
#include "tfm_peripherals_def.h"
#include "tfm_log.h"

#ifdef IFX_FAULTS_INFO_DUMP
void ifx_faults_dump(void)
{
    while (true) {
        cy_en_SysFault_source_t fault_source = Cy_SysFault_GetErrorSource(IFX_TFM_FAULT_STRUCT);
        if ((uint8_t)fault_source == CY_SYSFAULT_NO_FAULT) {
            break;
        }

        ERROR_RAW("Platform Exception: 0x%08x\n", fault_source);

        switch (fault_source) {
            case PERI_0_PERI_MS0_PPC_VIO:
            case PERI_0_PERI_MS1_PPC_VIO:
            case PERI_1_PERI_MS0_PPC_VIO:
            case PERI_1_PERI_MS1_PPC_VIO: {
                ifx_faults_dump_peri_msx_ppc_vio_fault(IFX_TFM_FAULT_STRUCT);
                break;
            }

            case PERI_0_PERI_PPC_PC_MASK_VIO:
            case PERI_1_PERI_PPC_PC_MASK_VIO: {
                ifx_faults_dump_peri_ppc_pc_mask_vio_fault(IFX_TFM_FAULT_STRUCT);
                break;
            }

            case PERI_0_PERI_GP1_TIMEOUT_VIO:
            case PERI_0_PERI_GP2_TIMEOUT_VIO:
            case PERI_0_PERI_GP3_TIMEOUT_VIO:
            case PERI_0_PERI_GP4_TIMEOUT_VIO:
            case PERI_0_PERI_GP5_TIMEOUT_VIO:
            case PERI_1_PERI_GP1_TIMEOUT_VIO:
            case PERI_1_PERI_GP2_TIMEOUT_VIO:
            case PERI_1_PERI_GP3_TIMEOUT_VIO:
            case PERI_1_PERI_GP4_TIMEOUT_VIO: {
                ifx_faults_dump_peri_gpx_timeout_vio_fault(IFX_TFM_FAULT_STRUCT);
                break;
            }

            case PERI_0_PERI_GP0_AHB_VIO:
            case PERI_0_PERI_GP1_AHB_VIO:
            case PERI_0_PERI_GP2_AHB_VIO:
            case PERI_0_PERI_GP3_AHB_VIO:
            case PERI_0_PERI_GP4_AHB_VIO:
            case PERI_0_PERI_GP5_AHB_VIO:
            case PERI_1_PERI_GP0_AHB_VIO:
            case PERI_1_PERI_GP1_AHB_VIO:
            case PERI_1_PERI_GP2_AHB_VIO:
            case PERI_1_PERI_GP3_AHB_VIO:
            case PERI_1_PERI_GP4_AHB_VIO: {
                ifx_faults_dump_peri_gpx_ahb_vio_fault(IFX_TFM_FAULT_STRUCT);
                break;
            }

            /* MPC fault sources common to all CAT1D devices */
            case SMIF_0_FAULT_MXSMIF_TOP:
            case SMIF_1_FAULT_MXSMIF_TOP:

            /* Some MPC fault enumerators are named differently across IP generations */

            /* RRAM controller host interface MPC fault */
#if (CY_IP_MXS22RRAMC_VERSION >= 2)
            case MXRRAMC_RRAMC_HOST_IF_MPC_FAULT:
#else /* (CY_IP_MXS22RRAMC_VERSION >= 2) */
            case M33SYSCPUSS_RRAMC_HOST_IF_MPC_FAULT:
#endif /* (CY_IP_MXS22RRAMC_VERSION >= 2) */

            /* SRAM controller MPC fault */
#if (CY_IP_MXSRAMC_VERSION >= 3)
            case MXSRAMC_0_MPC_MMIO_VIO:
#else /* (CY_IP_MXSRAMC_VERSION >= 3) */
            case M33SYSCPUSS_RAMC0_MPC_FAULT_MMIO:
            case M33SYSCPUSS_RAMC1_MPC_FAULT_MMIO:
#endif /* (CY_IP_MXSRAMC_VERSION >= 3) */

            /* SOCMEM MPC fault */
#if (CY_IP_MXSOCMEM_VERSION >= 3)
            case SOCMEM_0_SOCMEM_PD0_MPC:
            case SOCMEM_0_SOCMEM_PD1_MPC:
#else /* (CY_IP_MXSOCMEM_VERSION >= 3) */
            case SOCMEM_SOCMEM_MPC:
#endif /* (CY_IP_MXSOCMEM_VERSION >= 3) */
            {
                ifx_faults_dump_mpc_fault(IFX_TFM_FAULT_STRUCT);
                break;
            }

            default: {
                ifx_faults_dump_default_fault(IFX_TFM_FAULT_STRUCT);
            }
        }

        Cy_SysFault_ClearStatus(IFX_TFM_FAULT_STRUCT);
    }
}
#endif /* IFX_FAULTS_INFO_DUMP */
