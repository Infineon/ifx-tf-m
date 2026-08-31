/*
 * (c) 2025-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * \file protection_mpc_sert.h
 * \brief This file contains the API used by HW MPC driver, to protect memory via SE RT Basic/Full.
 */

#ifndef PROTECTION_MPC_SERT_H
#define PROTECTION_MPC_SERT_H

#include "config_impl.h"
#include "config_tfm.h"
#include "fih.h"
#include "tfm_hal_defs.h"
#include "protection_types.h"

/*
 * External MPC attribute cache
 * ----------------------------
 * The SE RT Services own certain MPCs (e.g. RRAM, SRAM) that TF-M can neither
 * program nor read back directly. For every such memory a software attribute
 * cache mirrors the configuration applied through the SE RT service so that
 * ifx_mpc_sert_memory_check() can validate accesses.
 *
 * Each cache is an array of 32-bit words, one word per MPC block. Within a word
 * the attributes are stored as one 4-bit field per protection context, emulating
 * the MPC HW structure:
 *   2:0  - PC0 W/R/NS      18:16 - PC4 W/R/NS
 *   6:4  - PC1 W/R/NS      22:20 - PC5 W/R/NS
 *  10:8  - PC2 W/R/NS      26:24 - PC6 W/R/NS
 *  14:12 - PC3 W/R/NS      30:28 - PC7 W/R/NS
 *
 * For each nibble:
 * - bit[0] : NS (0 - secure, 1 - non-secure)
 * - bit[1] : R  (0 - read not allowed, 1 - read allowed)
 * - bit[2] : W  (0 - write not allowed, 1 - write allowed)
 */

/* Convert a cy_en_mpc_size_t block size to its size in bytes. */
#define IFX_MPC_EXT_BLOCK_SIZE_TO_BYTES(mpc_size)   (1UL << ((uint32_t)(mpc_size) + 5UL))

/* Number of bits used to cache the attributes of a single protection context. */
#define IFX_MPC_EXT_CACHE_PC_FIELD_BITS        (4U)

/* Returns the starting bit position of the cached field for the selected PC. */
#define IFX_MPC_EXT_CACHE_PC_FIELD_SHIFT(pc)   ((uint32_t)(pc) * IFX_MPC_EXT_CACHE_PC_FIELD_BITS)

/* Bit positions within each cached protection-context field. */
#define IFX_MPC_EXT_CACHE_NS_BIT               (0U)
#define IFX_MPC_EXT_CACHE_READ_BIT             (1U)
#define IFX_MPC_EXT_CACHE_WRITE_BIT            (2U)

/* Encodes a single cached permission bit for the selected PC. */
#define IFX_MPC_EXT_CACHE_PC_BIT(pc, bit)      \
    (1UL << (IFX_MPC_EXT_CACHE_PC_FIELD_SHIFT(pc) + (uint32_t)(bit)))

/* Number of 32-bit cache words required to describe \p mem_size bytes of memory
 * whose MPC uses \p block_size (cy_en_mpc_size_t) blocks. Platforms use this to
 * size the attribute cache storage of an \ref ifx_mpc_sert_mem_t entry. */
#define IFX_MPC_SERT_CACHE_SIZE(mem_size, block_size) \
    ((mem_size) / IFX_MPC_EXT_BLOCK_SIZE_TO_BYTES(block_size))

/**
 * \brief Descriptor of an external memory whose MPC is handled by SE RT Services.
 *
 * Platforms provide one entry per external MPC in \ref ifx_mpc_sert_memories.
 */
typedef struct {
    const ifx_memory_config_t *mem;          /**< MPC identity, block size, base and size */
    uint32_t                  *attr_cache;   /**< Cache storage, block_count words */
    uint32_t                   block_count;  /**< Number of MPC blocks (equal to attr_cache words) */
#ifdef TFM_FIH_PROFILE_ON
    uint32_t                  *attr_cache_inv; /**< Per-word complement of attr_cache for integrity checks */
#endif /* TFM_FIH_PROFILE_ON */
} ifx_mpc_sert_mem_t;

/** Platform-provided table of external memories handled via SE RT Services. */
extern const ifx_mpc_sert_mem_t ifx_mpc_sert_memories[];

/** Number of entries in \ref ifx_mpc_sert_memories. */
extern const size_t ifx_mpc_sert_memories_count;

/**
 * \brief Initialize the MPC attribute caches for SE RT Services.
 *
 * This function sets up the attribute cache of every external memory listed in
 * \ref ifx_mpc_sert_memories to its reset defaults. The caches are used for
 * memory protection checks.
 *
 * \return TFM_HAL_SUCCESS on success, error code otherwise.
 */
FIH_RET_TYPE(enum tfm_hal_status_t) ifx_mpc_sert_init(void);

/**
 * \brief Apply MPC configuration for MPC controller that is handled by optional
 * SE RT Service or other non-TF-M service/hardware.
 *
 * \param[in] mpc_reg_cfg   MPC configuration structure which will be applied.
 *                          See \ref ifx_mpc_raw_region_config_t for more details.
 *
 * \return    TFM_HAL_SUCCESS             - MPC configuration applied successfully.
 *            TFM_HAL_ERROR_GENERIC       - failed to apply MPC configuration.
 *            TFM_HAL_ERROR_NOT_SUPPORTED - configuration can not be applied.
 *                                          Can occur when MPC that should
 *                                          protect the memory is not controlled
 *                                          by TFM.
 *            TFM_HAL_ERROR_INVALID_INPUT - invalid input (e.g. provided memory
 *                                          region is not supported by external
 *                                          service).
 */
enum tfm_hal_status_t ifx_mpc_sert_apply_configuration(
                                        const ifx_mpc_raw_region_config_t* mpc_reg_cfg);

/**
 * \brief Check memory access permissions using MPC attribute cache.
 *
 * This function validates access permissions for a memory region using the MPC cache.
 *
 * \param[in]  p_info         Partition information structure.
 * \param[in]  memory_config  Memory configuration structure.
 * \param[in]  base           Base address of the memory region.
 * \param[in]  size           Size of the memory region.
 * \param[in]  access_type    Requested access type (read/write/secure/non-secure).
 *
 * \return    TFM_HAL_SUCCESS         - Access is permitted.
 *            TFM_HAL_ERROR_MEM_FAULT - Access is not permitted or invalid input.
 */
FIH_RET_TYPE(enum tfm_hal_status_t) ifx_mpc_sert_memory_check(
                                           const struct ifx_partition_info_t *p_info,
                                           const ifx_memory_config_t* memory_config,
                                           uintptr_t base,
                                           size_t size,
                                           uint32_t access_type);

#ifdef TFM_FIH_PROFILE_ON
/**
 * \brief Verify MPC configuration using MPC attribute cache.
 *
 * This function checks that the MPC cache matches the expected configuration.
 *
 * \param[in] mpc_reg_cfg   MPC configuration structure to verify.
 *
 * \return    TFM_HAL_SUCCESS         - Configuration matches expected values.
 *            TFM_HAL_ERROR_GENERIC   - Configuration does not match or invalid input.
 */
FIH_RET_TYPE(enum tfm_hal_status_t) ifx_mpc_sert_verify_configuration(
                                        const ifx_mpc_raw_region_config_t* mpc_reg_cfg);
#endif /* TFM_FIH_PROFILE_ON */

#endif /* PROTECTION_MPC_SERT_H */
