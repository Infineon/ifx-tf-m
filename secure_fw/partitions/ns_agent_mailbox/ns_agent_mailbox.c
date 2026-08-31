/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 * (c) 2021-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "async.h"
#include "config_tfm.h"
#include "ns_agent_mailbox_defs.h"
#include "ns_agent_mailbox_signal_utils.h"
#include "psa/service.h"
#include "psa_manifest/ns_agent_mailbox.h"
#include "tfm_hal_mailbox.h"
#include "tfm_hal_multi_core.h"
#include "tfm_hal_platform.h"
#include "tfm_multi_core.h"
#include "tfm_rpc.h"
#include "tfm_log_unpriv.h"

#include "compiler_ext_defs.h" /* Keep last. */

/* Internal states */
enum mailbox_state {
    START,
    NS_CORE_RUNNING,
    LINK_ESTABLISHED,
};

static void boot_ns_core(void)
{
    /* Boot up non-secure core */
    VERBOSE_UNPRIV_RAW("Enabling non-secure core...\n");

    tfm_hal_boot_ns_cpu(tfm_hal_get_ns_VTOR_off_core());
    tfm_hal_wait_for_ns_cpu_ready();
}

static void state_transition(enum mailbox_state *state, const psa_msg_t *msg)
{
    switch (*state) {
    case START:
        if (msg->type == BOOT_NS_CORE) {
            boot_ns_core();

            if (tfm_inter_core_comm_init()) {
                ERROR_UNPRIV("Inter-core communication init failed\r\n");
                psa_panic();
            }

            *state = NS_CORE_RUNNING;
        } else {
            psa_panic();
        }
        break;

    case NS_CORE_RUNNING:
        if (msg->type == ENABLE_MAILBOX) {
            if (tfm_inter_core_comm_enable()) {
                ERROR_UNPRIV("Inter-core communication enable failed\r\n");
                psa_panic();
            }
            mailbox_enable_interrupts();

            *state = LINK_ESTABLISHED;
        } else if (msg->type == NS_CORE_SHUTDOWN) {
            if (tfm_inter_core_comm_deinit()) {
                ERROR_UNPRIV("Inter-core communication deinit failed\r\n");
                psa_panic();
            }

            *state = START;
        } else {
            psa_panic();
        }
        break;

    case LINK_ESTABLISHED:
        if (msg->type == DISABLE_MAILBOX) {
            mailbox_disable_interrupts();

            if (tfm_inter_core_comm_disable()) {
                ERROR_UNPRIV("Inter-core communication disable failed\r\n");
                psa_panic();
            }

            *state = NS_CORE_RUNNING;
        } else {
            psa_panic();
        }
        break;
    }
}

void ns_agent_mailbox_entry(void)
{
    enum mailbox_state state = START;
    psa_signal_t signals = 0, active_signal = 0;

#if CONFIG_TFM_AUTO_BOOT_NS_CORE
    boot_ns_core();

    if (tfm_inter_core_comm_init()) {
        ERROR_UNPRIV_RAW("Inter-core communication init failed\n");
        psa_panic();
    }

    if (tfm_inter_core_comm_enable()) {
        ERROR_UNPRIV_RAW("Inter-core communication enable failed\n");
        psa_panic();
    }

    mailbox_enable_interrupts();

    state = LINK_ESTABLISHED;
#endif

    while (1) {
        signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
        while (mailbox_signal_is_active(signals)) {
            active_signal = mailbox_signal_get_active(signals);
            psa_eoi(active_signal);
            /* Discard late-arriving signals */
            if (state == LINK_ESTABLISHED) {
                tfm_rpc_client_call_handler(active_signal);
            }
            signals &= ~active_signal;
        }
#if CONFIG_TFM_SPM_BACKEND_IPC == 1
        if (signals & ASYNC_MSG_REPLY) {
            if (state == LINK_ESTABLISHED) {
                tfm_rpc_client_call_reply();
            } else {
                /* If the mailbox is not active, this is a response to
                 * a request that we've already rejected, so clean up
                 * but don't actually try to send the response
                 */
                tfm_rpc_client_drop_reply();
            }
            signals &= ~ASYNC_MSG_REPLY;
        }
#endif
#if (CONFIG_TFM_HYBRID_PLAT_SCHED_TYPE == TFM_HYBRID_PLAT_SCHED_NSPE) || \
    (CONFIG_TFM_HYBRID_PLAT_SCHED_TYPE == TFM_HYBRID_PLAT_SCHED_BALANCED)
        if (signals & NS_AGENT_MBOX_PROCESS_NEW_MSG_SIGNAL) {
            psa_status_t status;
            psa_msg_t msg;
            uint32_t nr_msg;

            status = psa_get(NS_AGENT_MBOX_PROCESS_NEW_MSG_SIGNAL, &msg);
            if (status != PSA_SUCCESS) {
                continue;
            }

            tfm_multi_core_clear_mbox_irq();

            if (msg.type != PSA_IPC_CALL) {
                status = PSA_ERROR_NOT_SUPPORTED;
            } else if (msg.out_size[0] != 4) {
                status = PSA_ERROR_PROGRAMMER_ERROR;
            } else {
                status = (psa_status_t)tfm_rpc_client_process_new_msg(&nr_msg);
                if (status == PSA_SUCCESS) {
                    psa_write(msg.handle, 0, (const void *)&nr_msg, msg.out_size[0]);
                }
            }

            psa_reply(msg.handle, status);
            signals &= ~NS_AGENT_MBOX_PROCESS_NEW_MSG_SIGNAL;
        }
#endif
        if (signals & TFM_NS_AGENT_MAILBOX_SERVICE_SIGNAL) {
            psa_msg_t msg;
            psa_status_t status = psa_get(TFM_NS_AGENT_MAILBOX_SERVICE_SIGNAL, &msg);
            if (status != PSA_SUCCESS) {
                continue;
            }

            state_transition(&state, &msg);

            psa_reply(msg.handle, PSA_SUCCESS);

            signals &= ~TFM_NS_AGENT_MAILBOX_SERVICE_SIGNAL;
        }

        if (signals != 0) {
            /* Wrong signal asserted */
            psa_panic();
        }
    }
}
