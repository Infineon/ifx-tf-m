/*
 * (c) 2025-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef NS_AGENT_MAILBOX_INTERFACE_H
#define NS_AGENT_MAILBOX_INTERFACE_H

#include <stdint.h>
#include "psa/error.h"

/**
 * \brief Boot the non-secure core
 *
 * \return Returns values as specified by the \ref psa_status_t
 */
psa_status_t ns_agent_boot_ns_core(void);

/**
 * \brief Enable the mailbox
 *
 * \return Returns values as specified by the \ref psa_status_t
 */
psa_status_t ns_agent_mailbox_enable(void);

/**
 * \brief Disable the mailbox
 *
 * \return Returns values as specified by the \ref psa_status_t
 */
psa_status_t ns_agent_mailbox_disable(void);

/**
 * \brief Inform ns_agent_mailbox that the NS core has shut down
 *
 * Before re-enabling the mailbox, \ref ns_agent_boot_ns_core()
 * must be called.
 *
 * \return Returns values as specified by the \ref psa_status_t
 */
psa_status_t ns_agent_ns_core_shutdown(void);
#endif /* NS_AGENT_MAILBOX_INTERFACE_H */
