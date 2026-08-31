/*
 * (c) 2025-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "cmsis.h"
#include "provisioning.h"
#include "cy_device.h"
#include "coverity_check.h"

#if defined(CY_IP_MXS22SRSS)
/*
 * On devices using CY_IP_MXS22SRSS, the AP_CTL register is not accessible from
 * CM33. BootROM copies the OEM policy to a known memory location for firmware
 * to read.
 */

/* Debug port access control bitfields as specified by BootROM */
#define IFX_DEBUG_POLICY_CM33_AP_CTL_Pos            (0UL)
#define IFX_DEBUG_POLICY_CM33_AP_CTL_Msk            (0x007UL)
#define IFX_DEBUG_POLICY_CM55_AP_CTL_Pos            (3UL)
#define IFX_DEBUG_POLICY_CM55_AP_CTL_Msk            (0x038UL)
#define IFX_DEBUG_POLICY_SYS_AP_CTL_Pos             (6UL)
#define IFX_DEBUG_POLICY_SYS_AP_CTL_Msk             (0x1C0UL)

/* Debug port access configuration values as specified by BootROM */
#define IFX_DEBUG_POLICY_AP_DISABLE                 (0U)
#define IFX_DEBUG_POLICY_AP_ENABLE                  (1U)
#define IFX_DEBUG_POLICY_AP_ALLOW_FW                (2U)
#define IFX_DEBUG_POLICY_AP_ALLOW_CERT              (3U)
#define IFX_DEBUG_POLICY_AP_ALLOW_OPEN              (4U)

/* Key length in bytes. The length is fixed because only 256-bit ECDSA keys are
 * supported. Note the 1st byte defines key format, the following bytes are X
 * and Y points 32 bytes each.
 * As specified by BootROM.
 */
#define IFX_OEM_POLICY_PUB_KEY_SIZE                 (65U)

/* Address where ifx_oem_policy_t is stored as specified by BootROM */
#define IFX_OEM_POLICY_ADDR                         (0x12001100u)

/* Debug policy structure as specified by BootROM */
typedef struct
{
    /* Debug port access control and access restrictions for SYS-AP */
    uint32_t access_cfg;
    /* AP_CTL register values that reflect debug policy */
    uint32_t syscpuss_ap_ctl;  /* CM33-AP and SYS-AP control */
    uint32_t reserved;
} ifx_debug_policy_t;

/* OEM policy structure to keep OEM debug restrictions, debug key to open access port
 * using certificate and flag that indicates possibility to move to RMA LCS.
 * As specified by BootROM.
 */
typedef struct
{
    ifx_debug_policy_t dbg_policy;
    uint32_t rma;
    uint8_t debug_key[IFX_OEM_POLICY_PUB_KEY_SIZE];
    uint8_t padding[3];
    uint32_t boundary_scan;
    uint32_t warm_boot;
    uint32_t reserved;
} ifx_oem_policy_t;

ifx_dap_state_t ifx_get_dap_state(void)
{
    /* Read the BootROM-supplied policy word twice through a volatile pointer -
     * once direct, once complemented - so a single fault on the load cannot force
     * the SECURED-implying DISABLE result. FIH is unavailable here (partition
     * context), so the redundancy is done in plain C and fails to the worst case
     * (debug open) on any inconsistency. */
    volatile const ifx_oem_policy_t *policy =
        (volatile const ifx_oem_policy_t *)IFX_OEM_POLICY_ADDR;

    uint32_t access_cfg     = policy->dbg_policy.access_cfg;
    uint32_t access_cfg_inv = ~policy->dbg_policy.access_cfg;

    /* The two independent reads must be exact complements of each other. */
    if (access_cfg != (uint32_t)~access_cfg_inv) {
        return IFX_DAP_ENABLED_S_NS;
    }

    /* Get state of CM33 DAP from both the direct value and the complemented copy. */
    uint32_t cm33_acc_cfg     = _FLD2VAL(IFX_DEBUG_POLICY_CM33_AP_CTL, access_cfg);
    uint32_t cm33_acc_cfg_chk = _FLD2VAL(IFX_DEBUG_POLICY_CM33_AP_CTL, (uint32_t)~access_cfg_inv);

    switch (cm33_acc_cfg) {
        case IFX_DEBUG_POLICY_AP_DISABLE:
            /* Only branch that yields the SECURED claim: require the
             * independently-derived copy to confirm DISABLE as well. */
            if (cm33_acc_cfg_chk != IFX_DEBUG_POLICY_AP_DISABLE) {
                return IFX_DAP_ENABLED_S_NS;
            }
            return IFX_DAP_DISABLED;
        case IFX_DEBUG_POLICY_AP_ENABLE:
        case IFX_DEBUG_POLICY_AP_ALLOW_FW:
        case IFX_DEBUG_POLICY_AP_ALLOW_CERT:
        case IFX_DEBUG_POLICY_AP_ALLOW_OPEN:
            /* DAP ALLOW states are treated as DAP ENABLED as port might already
             * have been opened (e.g. using certificate) and TFM has no way to
             * check it. Policy does not provide S/NS debug details, thus assume
             * both S and NS debug is enabled */
            return IFX_DAP_ENABLED_S_NS;
        default:
            return IFX_DAP_UNKNOWN;
    }
}
#elif defined(CY_IP_MXS40SSRSS)
/*
 * On devices using CY_IP_MXS40SSRSS, the AP_CTL register is accessible from CM33, thus DAP state is
 * determined by reading the register directly.
 */

ifx_dap_state_t ifx_get_dap_state(void)
{
    /* Read AP_CTL twice - once direct, once complemented - so a single fault on
     * the load cannot force the SECURED-implying DISABLED result. FIH is
     * unavailable here (partition context), so the redundancy is done in plain C
     * and fails to the worst case (debug open) on any inconsistency. */
    volatile uint32_t ap_ctl     = CPUSS->AP_CTL;
    volatile uint32_t ap_ctl_inv = ~CPUSS->AP_CTL;

    /* The two independent reads must be exact complements of each other. */
    TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Rule_13_2, "Redundant volatile reads are intentional for fault detection; evaluation order does not matter")
    TFM_COVERITY_DEVIATE_LINE(cert_exp30_c, "Volatile variables do not have side-effects, so read order doesn't matter")
    if (ap_ctl != (uint32_t)~ap_ctl_inv) {
        return IFX_DAP_ENABLED_S_NS;
    }
    TFM_COVERITY_BLOCK_END(MISRA_C_2023_Rule_13_2)

    uint32_t cm33_disable     = _FLD2VAL(CPUSS_AP_CTL_CM33_0_DISABLE, ap_ctl);
    uint32_t cm33_disable_chk = _FLD2VAL(CPUSS_AP_CTL_CM33_0_DISABLE, (uint32_t)~ap_ctl_inv);

    if (cm33_disable != 0U) {
        /* Only branch that yields the SECURED claim: require the
         * independently-derived copy to confirm DISABLE as well. */
        if (cm33_disable_chk == 0U) {
            return IFX_DAP_ENABLED_S_NS;
        }
        /* CM33 AP is permanently disabled */
        return IFX_DAP_DISABLED;
    }

    /* CM33 AP is allowed/enabled - thus check Secure debugging capabilities */
    if (_FLD2VAL(CPUSS_AP_CTL_CM33_0_SECURE_DISABLE, ap_ctl)) {
        /* CM33 Secure Debugging is permanently disabled */
        return IFX_DAP_ENABLED_NS_ONLY;
    }

    /* CM33 Secure Debugging is allowed/enabled */
    return IFX_DAP_ENABLED_S_NS;
}

#else /* CY_IP_MXS22SRSS */
#error Unsupported device for ifx_get_dap_state()
#endif /* CY_IP_MXS22SRSS */
