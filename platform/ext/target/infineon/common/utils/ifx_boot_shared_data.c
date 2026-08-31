/*
 * (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "ifx_boot_shared_data.h"
#include "region_defs.h"
#include "tfm_boot_status.h"
#include "tfm_log.h"
#include "coverity_check.h"

#include <string.h>

#ifdef BOOT_DATA_AVAILABLE
void ifx_mock_boot_shared_data_if_not_present(void)
{
    struct shared_data_tlv_header mocked_boot_data;

    /* Mock only boot shared data header. This is enough for FWU and ATTESTATION
     * partitions initialization to pass. This way no misleading data will be
     * present in Attestation token or during FWU. */
    mocked_boot_data.tlv_magic = SHARED_DATA_TLV_INFO_MAGIC;
    /* length of data (including header) */
    mocked_boot_data.tlv_tot_len = (uint16_t)(sizeof(struct shared_data_tlv_header));

    /* Accept existing boot shared data only if its header is structurally
     * consistent: correct magic and a total length that spans at least the
     * header */
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_11_4, "Boot shared data area is accessed at a fixed address")
    const struct shared_data_tlv_header *hdr =
        (const struct shared_data_tlv_header *)BOOT_TFM_SHARED_DATA_BASE;

    if ((hdr->tlv_magic != (uint16_t)SHARED_DATA_TLV_INFO_MAGIC) ||
        (hdr->tlv_tot_len < SHARED_DATA_HEADER_SIZE)) {
        /* Boot shared data header is not present/not valid thus clean boot
         * shared data area and mock the header */
        (void)memset((uint32_t*)BOOT_TFM_SHARED_DATA_BASE, 0, BOOT_TFM_SHARED_DATA_SIZE);

        (void)memcpy((uint32_t*)BOOT_TFM_SHARED_DATA_BASE,
                     &mocked_boot_data,
                     sizeof(mocked_boot_data));

        /* Print appropriate warning */
        INFO_RAW("WARNING: Boot shared data is not present so TF-M will mock it.\n");
    }
}
#endif /* BOOT_DATA_AVAILABLE */
