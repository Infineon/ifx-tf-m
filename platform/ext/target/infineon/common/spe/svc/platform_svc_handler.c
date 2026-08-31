/*
 * (c) 2023-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "config_tfm.h"
#include "cmsis.h"
#include "current.h"
#include "cy_device.h"
#include "ifx_fih.h"
#include "platform_svc_api.h"
#include "platform_svc_private.h"
#include "spm.h"
#include "coverity_check.h"
#include "tfm_hal_isolation.h"
#include "tfm_peripherals_def.h"
#if IFX_UART_ENABLED
#include "uart_pdl_stdout.h"
#endif

static int32_t ifx_svc_platform_uart_log_handler(const char *msg, uint32_t len, uint32_t core_id)
{
#if IFX_UART_ENABLED
    TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Directive_4_6, "FIH uses basic types without size and signedness")
    TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Rule_15_6, "Definition of FIH_SET() is missing {}, but FIH_PANIC is always a simple statement")
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_10_1, "Definition of FIH_SET() treats fih_delay() returned int as a bool")
    IFX_FIH_DECLARE(enum tfm_hal_status_t, fih_rc, TFM_HAL_ERROR_GENERIC);
    TFM_COVERITY_BLOCK_END(MISRA_C_2023_Rule_15_6 MISRA_C_2023_Directive_4_6)
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_11_6, "External macro GET_CURRENT_COMPONENT roughly casts p_curr_thrd->p_context_ctrl")
    struct partition_t *curr_partition = GET_CURRENT_COMPONENT();

    TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Rule_10_3, "All parameters are converted corectly")
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_20_7, "Cannot wrap with parentheses due to Fault injection architecture and define FIH_RET_TYPE")
    FIH_CALL(tfm_hal_memory_check, fih_rc,
             curr_partition->boundary, (uintptr_t)msg,
             (size_t)len, (uint32_t)TFM_HAL_ACCESS_READABLE);
    TFM_COVERITY_BLOCK_END(MISRA_C_2023_Rule_10_3)
    TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Rule_10_4, "Cannot change types due to Fault injection architecture")
    TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Rule_20_7, "Cannot wrap with parentheses due to Fault injection architecture and define FIH_RET_TYPE")
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_10_1, "Cannot change not equal logic due to Fault injection architecture and define FIH_NOT_EQ")
    if (FIH_NOT_EQ(fih_rc, PSA_SUCCESS)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }
    TFM_COVERITY_BLOCK_END(MISRA_C_2023_Rule_20_7 MISRA_C_2023_Rule_10_4)

    (void)stdio_output_string_raw(msg, len, (ifx_stdio_core_id_t)core_id);

    return (int32_t)PSA_SUCCESS;
#else
    return (int32_t)PSA_ERROR_NOT_SUPPORTED;
#endif
}

#ifdef IFX_TFM_TEST_SYSTICK_SVC
static int32_t ifx_svc_platform_enable_systick(uint32_t enable)
{
    if (enable != 0u) {
        /* Enable SysTick every 100 cycles of clock to not overload the system
         * with interrupts */
        (void)SysTick_Config(SystemCoreClock / 100u);
        /* SysTick_Config set the lowest IRQ priority (equal to PendSV) - which
         * generate tfm_core_panic(). So, we should fix this. */
        NVIC_SetPriority(TFM_TIMER0_IRQ, DEFAULT_IRQ_PRIORITY);
    }
    else {
        /* Disable SysTick IRQ and SysTick Timer */
        SysTick->CTRL  &= ~(SysTick_CTRL_TICKINT_Msk |
                         SysTick_CTRL_ENABLE_Msk);
    }

    return (int32_t)PSA_SUCCESS;
}
#endif /* IFX_TFM_TEST_SYSTICK_SVC */

static int32_t ifx_svc_platform_system_reset(void)
{
    NVIC_SystemReset();

    /* Suppress Pe111 (statement is unreachable) and Pe128 (loop is not
     * reachable) for IAR */
#if defined(__ICCARM__)
#pragma diag_suppress = Pe111,Pe128
#endif
    /* NVIC_SystemReset() is __NO_RETURN; if a fault skips the reset, trap here
     * rather than returning control to the caller. */
    for (;;) {
        __NOP();
    }
#if defined(__ICCARM__)
#pragma diag_default = Pe111,Pe128
#endif
}

static int32_t ifx_svc_platform_original_vectors(psa_handle_t msg_handle, ifx_original_iovec_t *io_vec)
{
    struct connection_t *handle = NULL;

    /* It is a fatal error if message handle is invalid */
    handle = spm_msg_handle_to_connection(msg_handle);
    if (handle == NULL) {
        tfm_core_panic();
    }

    /*
     * It is a fatal error if message handle does not refer to a request
     * message
     */
    if (handle->msg.type < PSA_IPC_CALL) {
        tfm_core_panic();
    }

    TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Directive_4_6, "FIH uses basic types without size and signedness")
    TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Rule_15_6, "Definition of FIH_SET() is missing {}, but FIH_PANIC is always a simple statement")
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_10_1, "Definition of FIH_SET() treats fih_delay() returned int as a bool")
    IFX_FIH_DECLARE(enum tfm_hal_status_t, fih_rc, TFM_HAL_ERROR_GENERIC);
    TFM_COVERITY_BLOCK_END(MISRA_C_2023_Rule_15_6 MISRA_C_2023_Directive_4_6)
    struct partition_t *curr_partition = GET_CURRENT_COMPONENT();

    /* Check that the partition can write to io_vec */
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_20_7, "Cannot wrap with parentheses due to Fault injection architecture and define FIH_RET_TYPE")
    FIH_CALL(tfm_hal_memory_check,
             fih_rc,
             curr_partition->boundary,
             (uintptr_t)io_vec,
             sizeof(*io_vec),
             (uint32_t)TFM_HAL_ACCESS_READWRITE);
    TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Rule_10_4, "Cannot change types due to Fault injection architecture")
    TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Rule_20_7, "Cannot wrap with parentheses due to Fault injection architecture and define FIH_RET_TYPE")
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_10_1, "Cannot change not equal logic due to Fault injection architecture and define FIH_NOT_EQ")
    if (FIH_NOT_EQ(fih_rc, PSA_SUCCESS)) {
        return (int32_t)PSA_ERROR_PROGRAMMER_ERROR;
    }
    TFM_COVERITY_BLOCK_END(MISRA_C_2023_Rule_20_7 MISRA_C_2023_Rule_10_4)

    size_t invec_num = PSA_MAX_IOVEC;
    size_t outvec_num = PSA_MAX_IOVEC;
    volatile size_t invec_steps = 0u;
    volatile size_t outvec_steps = 0u;

    /* Count number of in and out vectors */
    while ((invec_num > 0u) && (handle->msg.in_size[invec_num - 1u] == 0u)) {
        invec_num--;
        invec_steps++;
    }
    while ((outvec_num > 0u) && (handle->msg.out_size[outvec_num - 1u] == 0u)) {
        outvec_num--;
        outvec_steps++;
    }

    /* Each iteration decrements the count and increments its step counter, so the
     * sum stays PSA_MAX_IOVEC only if every iteration ran to completion; a skipped
     * or glitch-truncated loop breaks the identity. */
    if (((invec_num + invec_steps) != PSA_MAX_IOVEC) ||
        ((outvec_num + outvec_steps) != PSA_MAX_IOVEC)) {
        tfm_core_panic();
    }

    for (size_t i = 0; i < invec_num; i++) {
        /* Copy the original invec data */
        io_vec->invec[i].base = handle->invec_base[i];
        io_vec->invec[i].len = handle->msg.in_size[i];
    }

    for (size_t i = 0; i < outvec_num; i++) {
        /* Copy the original outvec data */
        io_vec->outvec[i].base = handle->outvec_base[i];
        io_vec->outvec[i].len = handle->msg.out_size[i];
    }

    io_vec->invec_count = invec_num;
    io_vec->outvec_count = outvec_num;

    return (int32_t)PSA_SUCCESS;
}

TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_8_4, "Prototype in tfm_svcalls.c")
int32_t platform_svc_handlers(uint8_t svc_num, uint32_t *svc_args,
                                uint32_t lr)
{
    (void) lr;

    int32_t retval;

    switch (svc_num) {
        case IFX_SVC_PLATFORM_UART_LOG:
            retval = ifx_svc_platform_uart_log_handler(
                    (const char *)svc_args[0], svc_args[1], svc_args[2]);
            break;

#ifdef IFX_TFM_TEST_SYSTICK_SVC
        case IFX_SVC_PLATFORM_ENABLE_SYSTICK:
            retval = ifx_svc_platform_enable_systick(svc_args[0]);
            break;
#endif /* IFX_TFM_TEST_SYSTICK_SVC */

        case IFX_SVC_PLATFORM_SYSTEM_RESET:
            retval = ifx_svc_platform_system_reset();
            break;

        case IFX_SVC_PLATFORM_ORIGINAL_IOVEC:
            retval = ifx_svc_platform_original_vectors(svc_args[0], (ifx_original_iovec_t *)svc_args[1]);
            break;

        default:
            retval = PSA_ERROR_GENERIC_ERROR;
            break;
    }

    return retval;
}
