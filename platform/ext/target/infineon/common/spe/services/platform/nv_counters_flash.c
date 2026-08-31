/*
 * (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "ifx_se_crc32.h"
#include "ifx_utils.h"
#include "nv_counters_flash_driver.h"
#include "coverity_check.h"
#include "tfm_plat_nv_counters.h"

#include <limits.h>
#include <stddef.h>

/* Compilation time checks to be sure the defines are well defined */
#ifndef IFX_TFM_NV_COUNTERS_SECTOR_SIZE
#error "Flash driver sector size (IFX_TFM_NV_COUNTERS_SECTOR_SIZE) must be defined"
#endif

#ifndef IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE
#error "Flash driver instance (IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE) must be defined"
#endif
/* End of compilation time checks to be sure the defines are well defined */

#define IFX_NV_COUNTER_SIZE  sizeof(uint32_t)
/* The in-RAM, CRC-protected counter block is sized to the counters actually in
 * use, not to the whole flash sector, then rounded up to the flash program unit
 * (the driver only programs whole program-units and does the full-row
 * read-modify-write internally). Only this block is read/programmed; the rest of
 * the physical sector is left untouched. */
#define IFX_NUM_NV_COUNTERS  ((uint32_t)PLAT_NV_COUNTER_MAX)

/* Meaningful content: checksum + init_value + the counters in use. */
#define IFX_NV_COUNTERS_CONTENT_SIZE  ((2U * sizeof(uint32_t)) + \
                                       (IFX_NUM_NV_COUNTERS * IFX_NV_COUNTER_SIZE))
/* Programmed/CRC'd block, rounded up to the flash program unit. */
#define IFX_NV_COUNTERS_BLOCK_SIZE    IFX_ROUND_UP_TO_MULTIPLE(IFX_NV_COUNTERS_CONTENT_SIZE, \
                                                              IFX_TFM_NV_COUNTERS_PROGRAM_UNIT)

#define IFX_NV_COUNTERS_INITIALIZED 0xC0DE0042U

/* Tamper-independent "initialization done" marker. It lives in its own flash
 * sector that counter updates never erase, so it survives corruption of the
 * main/backup counter sectors and lets recovery tell a device being
 * initialized for the first time from one whose counter sectors were glitched
 * or torn. */
#define IFX_NVM_INIT_DONE_FLAG      0xAA5533CCU

#define IFX_TFM_NV_COUNTERS_AREA_OFFSET     (0U)
#define IFX_TFM_NV_COUNTERS_BACKUP_OFFSET   (IFX_TFM_NV_COUNTERS_AREA_OFFSET \
                                             + IFX_TFM_NV_COUNTERS_SECTOR_SIZE)
#define IFX_TFM_NV_COUNTERS_INIT_DONE_OFFSET (IFX_TFM_NV_COUNTERS_BACKUP_OFFSET \
                                             + IFX_TFM_NV_COUNTERS_SECTOR_SIZE)

#define IFX_NV_COUNTERS_CRC_INIT            (0)

/**
 * \brief Struct representing the NV counter data in flash.
 */
typedef struct ifx_nv_counters {
    uint32_t checksum;
    uint32_t init_value; /* Watermark to indicate if the NV counters have been initialised */
    union {
        uint32_t    counters[IFX_NUM_NV_COUNTERS]; /**< Array of NV counters */
        /* Spans the program-unit-rounded block; bytes past the counters are
         * zeroed padding included in the checksum. */
        uint8_t     bytes[IFX_NV_COUNTERS_BLOCK_SIZE - (2U * sizeof(uint32_t))];
    } nv_cnt;
} ifx_nv_counters_t;

/* The init-done marker lives in its own sector but, like the counter block,
 * must be programmed as a whole flash program unit. */
#define IFX_NV_INIT_DONE_BLOCK_SIZE   IFX_ROUND_UP_TO_MULTIPLE(sizeof(uint32_t), \
                                                              IFX_TFM_NV_COUNTERS_PROGRAM_UNIT)
typedef union ifx_nv_init_done {
    uint32_t flag;
    uint8_t  block[IFX_NV_INIT_DONE_BLOCK_SIZE];
} ifx_nv_init_done_t;

/*******************************************************************************
* Function Name: ifx_flash_counters_valid
********************************************************************************
* Checks if checksum and initial value of the counter or its backup is valid.
* Checksum is calculated over data and its address.
*
* \param nv_counters    Pointer to the counters.
*
* \return               true if the checksum matches, false otherwise.
*******************************************************************************/
static bool ifx_flash_counters_valid(const ifx_nv_counters_t *nv_counters)
{
    bool valid = false;
    uint32_t checksum = ifx_se_crc32d6a(sizeof(nv_counters->nv_cnt.bytes),
                                        nv_counters->nv_cnt.bytes,
                                        IFX_NV_COUNTERS_CRC_INIT);

    if ((nv_counters->init_value == IFX_NV_COUNTERS_INITIALIZED) &&
        (checksum == nv_counters->checksum)) {
        valid = true;
    }

    return valid;
}

static void ifx_flash_counter_set_checksum(ifx_nv_counters_t *nv_counters)
{
    nv_counters->checksum = ifx_se_crc32d6a(sizeof(nv_counters->nv_cnt.bytes),
                                            nv_counters->nv_cnt.bytes,
                                            IFX_NV_COUNTERS_CRC_INIT);
}

/*******************************************************************************
* Function Name: ifx_flash_program_and_verify
********************************************************************************
* Programs a counter sector and reads it back to confirm the flash cells hold
* the expected content. Guards against a glitched or partial program that the
* driver reports as successful.
*
* \param offset       Flash offset of the sector to program.
* \param nv_counters  Pointer to the counter block to write.
*
* \return             TFM_PLAT_ERR_SUCCESS if the read-back matches,
*                     TFM_PLAT_ERR_SYSTEM_ERR otherwise.
*******************************************************************************/
static enum tfm_plat_err_t ifx_flash_program_and_verify(uint32_t offset,
                                                        const ifx_nv_counters_t *nv_counters)
{
    int32_t ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.ProgramData(offset,
                                                                   nv_counters,
                                                                   sizeof(*nv_counters));
    if ((ret < 0) || (ret != (int32_t)sizeof(*nv_counters))) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Read back and verify the programmed block is self-consistent and its
     * checksum matches the block we intended to write, to detect a glitched or
     * partial write the driver reported as successful. */
    ifx_nv_counters_t check;
    ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.ReadData(offset, &check, sizeof(check));
    if ((ret < 0) || (ret != (int32_t)sizeof(check))) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    if (!ifx_flash_counters_valid(&check) ||
        (check.checksum != nv_counters->checksum)) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    return TFM_PLAT_ERR_SUCCESS;
}


enum tfm_plat_err_t tfm_plat_init_nv_counter(void)
{
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_2_2, "This code is added to perform compile time check")
    IFX_ASSERT(IFX_NUM_NV_COUNTERS >= PLAT_NV_COUNTER_MAX);
    /* The counter and init-done blocks must each fit within one physical sector. */
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_2_2, "This code is added to perform compile time check")
    IFX_ASSERT(sizeof(ifx_nv_counters_t) <= IFX_TFM_NV_COUNTERS_SECTOR_SIZE);
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_2_2, "This code is added to perform compile time check")
    IFX_ASSERT(sizeof(ifx_nv_init_done_t) <= IFX_TFM_NV_COUNTERS_SECTOR_SIZE);

    ifx_nv_counters_t nv_counters;
    int32_t ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.Initialize(NULL);
    if (ret != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Use the main counter sector if it is valid. */
    ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.ReadData(IFX_TFM_NV_COUNTERS_AREA_OFFSET,
                                                        &nv_counters,
                                                        sizeof(nv_counters));
    if ((ret < 0) || (ret != (int32_t)sizeof(nv_counters))) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    if (ifx_flash_counters_valid(&nv_counters)) {
        return TFM_PLAT_ERR_SUCCESS;
    }

    /* Main is invalid, fall back to the backup sector. */
    ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.ReadData(IFX_TFM_NV_COUNTERS_BACKUP_OFFSET,
                                                        &nv_counters,
                                                        sizeof(nv_counters));
    if ((ret < 0) || (ret != (int32_t)sizeof(nv_counters))) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    if (ifx_flash_counters_valid(&nv_counters)) {
        /* Backup is valid: restore the main sector from it. The main sector is
         * only erased once the backup has been validated. */
        ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.EraseSector(IFX_TFM_NV_COUNTERS_AREA_OFFSET);
        if (ret != ARM_DRIVER_OK) {
            return TFM_PLAT_ERR_SYSTEM_ERR;
        }
        return ifx_flash_program_and_verify(IFX_TFM_NV_COUNTERS_AREA_OFFSET, &nv_counters);
    }

    /* Both the main and backup sectors are invalid. Consult the init-done
     * marker in its dedicated sector: if this device was already initialized
     * once, fail secure instead of silently resetting the rollback counters to
     * zero. */
    uint32_t init_done = 0U;
    ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.ReadData(IFX_TFM_NV_COUNTERS_INIT_DONE_OFFSET,
                                                        &init_done,
                                                        sizeof(init_done));
    if ((ret < 0) || (ret != (int32_t)sizeof(init_done))) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }
    if (init_done == IFX_NVM_INIT_DONE_FLAG) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* First-time initialization: set all counters to zero in both the main and
     * backup sectors, then latch the init-done marker. */
    nv_counters.init_value = IFX_NV_COUNTERS_INITIALIZED;
    /* Zero the counters and any program-unit padding so the block and its
     * checksum are deterministic and no stale stack bytes reach flash. */
    for (uint32_t i = 0U; i < sizeof(nv_counters.nv_cnt.bytes); i++) {
        nv_counters.nv_cnt.bytes[i] = 0U;
    }
    ifx_flash_counter_set_checksum(&nv_counters);

    ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.EraseSector(IFX_TFM_NV_COUNTERS_AREA_OFFSET);
    if (ret != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }
    enum tfm_plat_err_t err = ifx_flash_program_and_verify(IFX_TFM_NV_COUNTERS_AREA_OFFSET,
                                                           &nv_counters);
    if (err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.EraseSector(IFX_TFM_NV_COUNTERS_BACKUP_OFFSET);
    if (ret != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }
    err = ifx_flash_program_and_verify(IFX_TFM_NV_COUNTERS_BACKUP_OFFSET, &nv_counters);
    if (err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    /* Latch the init-done marker last so an interrupted init is retried. */
    ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.EraseSector(IFX_TFM_NV_COUNTERS_INIT_DONE_OFFSET);
    if (ret != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }
    ifx_nv_init_done_t init_done_block;
    for (uint32_t i = 0U; i < sizeof(init_done_block.block); i++) {
        init_done_block.block[i] = 0U;
    }
    init_done_block.flag = IFX_NVM_INIT_DONE_FLAG;
    ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.ProgramData(IFX_TFM_NV_COUNTERS_INIT_DONE_OFFSET,
                                                           &init_done_block,
                                                           sizeof(init_done_block));
    if ((ret < 0) || (ret != (int32_t)sizeof(init_done_block))) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_read_nv_counter(enum tfm_nv_counter_t counter_id,
                                             uint32_t size, uint8_t *val)
{
    ifx_nv_counters_t nv_counters;
    uint32_t nv_counter_id = (uint32_t)counter_id;

    if (nv_counter_id >= IFX_NUM_NV_COUNTERS) {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }

    if (size != IFX_NV_COUNTER_SIZE) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Read the whole sector and validate its integrity before trusting any
     * counter value from it. */
    int32_t ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.ReadData(IFX_TFM_NV_COUNTERS_AREA_OFFSET,
                                                                &nv_counters,
                                                                sizeof(nv_counters));
    if ((ret < 0) || (ret != (int32_t)sizeof(nv_counters))) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    if (!ifx_flash_counters_valid(&nv_counters)) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    for (uint32_t i = 0U; i < IFX_NV_COUNTER_SIZE; i++) {
        val[i] = nv_counters.nv_cnt.bytes[(nv_counter_id * IFX_NV_COUNTER_SIZE) + i];
    }

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_set_nv_counter(enum tfm_nv_counter_t counter_id,
                                            uint32_t value)
{
    ifx_nv_counters_t nv_counters;
    uint32_t nv_counter_id = (uint32_t)counter_id;

    if (nv_counter_id >= IFX_NUM_NV_COUNTERS) {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }

    /* Read the whole sector so we can write it back to flash later */
    int32_t ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.ReadData(IFX_TFM_NV_COUNTERS_AREA_OFFSET,
                                                                &nv_counters,
                                                                sizeof(nv_counters));
    if ((ret < 0) || (ret != (int32_t)sizeof(nv_counters))) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Validate the stored sector before trusting the current counter value. */
    if (!ifx_flash_counters_valid(&nv_counters)) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    uint32_t old_value = nv_counters.nv_cnt.counters[nv_counter_id];

    if (value == old_value) {
        return TFM_PLAT_ERR_SUCCESS;
    }

    /* NV counters are monotonic. Guard the increment with a redundant,
     * complementary comparison so a single fault on one test cannot let a
     * lower value (rollback) through. */
    if ((value < old_value) || !(value > old_value)) {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }

    /* Erase backup sector */
    ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.EraseSector(IFX_TFM_NV_COUNTERS_BACKUP_OFFSET);
    if (ret != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    nv_counters.nv_cnt.counters[nv_counter_id] = value;

    ifx_flash_counter_set_checksum(&nv_counters);

    /* write sector data to backup sector */
    enum tfm_plat_err_t err = ifx_flash_program_and_verify(IFX_TFM_NV_COUNTERS_BACKUP_OFFSET,
                                                           &nv_counters);
    if (err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    /* Erase sector before writing to it */
    ret = IFX_NV_COUNTERS_CMSIS_FLASH_INSTANCE.EraseSector(IFX_TFM_NV_COUNTERS_AREA_OFFSET);
    if (ret != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Write the in-memory block content after modification to flash */
    return ifx_flash_program_and_verify(IFX_TFM_NV_COUNTERS_AREA_OFFSET, &nv_counters);
}

enum tfm_plat_err_t tfm_plat_increment_nv_counter(enum tfm_nv_counter_t counter_id)
{
    uint32_t security_cnt;

    enum tfm_plat_err_t err = tfm_plat_read_nv_counter(counter_id,
                                                       sizeof(security_cnt),
                                                       (uint8_t *)&security_cnt);
    if (err != TFM_PLAT_ERR_SUCCESS) {
        return err;
    }

    if (security_cnt == UINT32_MAX) {
        return TFM_PLAT_ERR_MAX_VALUE;
    }

    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Directive_4_7, "It's expected that errors are handled by the caller");
    return tfm_plat_set_nv_counter(counter_id, security_cnt + 1U);
}
