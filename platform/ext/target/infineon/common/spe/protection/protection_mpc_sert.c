/*
 * (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "config_impl.h"
#include "config_tfm.h"
#include "cy_mpc.h"
#include "ifx_se_platform.h"
#include "ifx_se_tfm_utils.h"
#include "protection_mpc_sert.h"
#include "region_defs.h"
#include "coverity_check.h"
#include "tfm_hal_isolation.h"

/* Default value used to initialize cache attributes before populating them. */
#define IFX_MPC_EXT_CACHE_ATTR_DEFAULT         (0U)

/* Reset attributes cached for every SE RT controlled memory: PC2/S (SPM) has
 * read access by default; all other protection contexts have none. */
#define IFX_MPC_SERT_DEFAULT_ATTR \
    IFX_MPC_EXT_CACHE_PC_BIT(IFX_PC_TFM_SPM_ID, IFX_MPC_EXT_CACHE_READ_BIT)

/* Converted information provided by ifx_mpc_raw_region_config_t to external MPC cache attributes */
typedef struct {
    /*! Bit field with MPC cached attributes */
    uint32_t attr;
    /*! Cache-bit mask derived from pc_apply_mask for the selected PCs. */
    uint32_t pc_cache_mask;
    /*! First block index in cached attributes */
    uint32_t first_block_idx;
    /*! Last block index in cached attributes */
    uint32_t last_block_idx;
} ifx_mpc_ext_cache_cfg_info_t;

/**
 * \brief Find the external memory descriptor whose MPC matches \p mpc.
 *
 * \param[in] mpc   MPC controller pointer used as the lookup key.
 *
 * \return Pointer to the matching descriptor, or NULL when none is found.
 */
static const ifx_mpc_sert_mem_t *ifx_mpc_sert_find_mem(const MPC_Type *mpc)
{
    for (size_t idx = 0U; idx < ifx_mpc_sert_memories_count; idx++) {
        if (ifx_mpc_sert_memories[idx].mem->mpc == mpc) {
            return &ifx_mpc_sert_memories[idx];
        }
    }

    return NULL;
}

/**
 * \brief Returns attributes for MPC attributes cache
 *
 * \param[in] sert_mem      External memory descriptor whose MPC is configured.
 * \param[in] mpc_reg_cfg   MPC configuration structure which will be applied.
 *                          See \ref ifx_mpc_raw_region_config_t for more details.
 * \param[out] info         Information generated for requested configuration.
 *
 * \return    TFM_HAL_SUCCESS             - MPC configuration applied successfully.
 *            TFM_HAL_ERROR_INVALID_INPUT - invalid input (e.g. invalid arguments or
 *                                          configuration).
 */
static enum tfm_hal_status_t ifx_mpc_sert_config_to_cache_info(
                                        const ifx_mpc_sert_mem_t *sert_mem,
                                        const ifx_mpc_raw_region_config_t *mpc_reg_cfg,
                                        ifx_mpc_ext_cache_cfg_info_t *info)
{
    if (info == NULL) {
        return TFM_HAL_ERROR_INVALID_INPUT;
    }

    /* Default values to improve FIH resistance */
    info->attr = IFX_MPC_EXT_CACHE_ATTR_DEFAULT;
    info->pc_cache_mask = IFX_MPC_EXT_CACHE_ATTR_DEFAULT;

    /* The configuration must target the same block size as the memory it
     * belongs to, otherwise the block indexing below would be inconsistent. */
    if (mpc_reg_cfg->mpc_block_size != sert_mem->mem->mpc_block_size) {
        return TFM_HAL_ERROR_INVALID_INPUT;
    }

    uint32_t block_size = IFX_MPC_EXT_BLOCK_SIZE_TO_BYTES(sert_mem->mem->mpc_block_size);
    uint32_t block_count = sert_mem->block_count;
    uint32_t offset = mpc_reg_cfg->offset;
    info->first_block_idx = offset / block_size;
    info->last_block_idx  = IFX_ROUND_UP_TO_MULTIPLE(offset + mpc_reg_cfg->size,
                                                     block_size)
                             / block_size;
    if ((info->first_block_idx >= block_count) ||
        (info->last_block_idx > block_count)) {
        return TFM_HAL_ERROR_INVALID_INPUT;
    }

    /* MPC config works with PC >= IFX_PC_TFM_SPM_ID. Lower PCs
     * are responsible for protecting/verifying their own resources. */
    for (uint32_t pc = IFX_PC_TFM_SPM_ID; pc < MPC_PC_NR; pc++) {
        /* Apply only the PCs explicitly requested by the configuration and
         * leave any other PC settings unchanged. */
        if (IFX_GET_PC(mpc_reg_cfg->pc_apply_mask, pc) == 0U) {
            continue;
        }

        /* Indicate that this PC has cached attributes */
        info->pc_cache_mask |= IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_NS_BIT) |
                               IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_READ_BIT) |
                               IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_WRITE_BIT);

        /* Cache secure/non-secure and read/write permissions for this PC. */
        if (IFX_GET_PC(mpc_reg_cfg->ns_mask, pc) != 0U) {
            info->attr |= IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_NS_BIT);
        }

        if (IFX_GET_PC(mpc_reg_cfg->r_mask, pc) != 0U) {
            info->attr |= IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_READ_BIT);
        }

        if (IFX_GET_PC(mpc_reg_cfg->w_mask, pc) != 0U) {
            info->attr |= IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_WRITE_BIT);
        }
    }

    return TFM_HAL_SUCCESS;
}

enum tfm_hal_status_t ifx_mpc_sert_apply_configuration(
                                        const ifx_mpc_raw_region_config_t* mpc_reg_cfg)
{
#if TFM_ISOLATION_LEVEL == 3
    /* SE RT Services are not available for SPM when TF-M scheduler is started.
     * It's expected to return success on dynamic switching. */
    if (ifx_asset_protection_type == IFX_ASSET_PROTECTION_PARTITION_DYNAMIC) {
        return TFM_HAL_SUCCESS;
    }
#endif /* TFM_ISOLATION_LEVEL == 3 */

    /* Resolve the external memory served by the requested MPC. */
    const ifx_mpc_sert_mem_t *sert_mem = ifx_mpc_sert_find_mem(mpc_reg_cfg->mpc_base);
    if (sert_mem == NULL) {
        return TFM_HAL_ERROR_INVALID_INPUT;
    }

    /* Validate the request and compute the cache update up front, before any
     * MPC programming happens, so the cache and hardware stay consistent. */
    ifx_mpc_ext_cache_cfg_info_t info;
    if (ifx_mpc_sert_config_to_cache_info(sert_mem, mpc_reg_cfg, &info) != TFM_HAL_SUCCESS) {
        return TFM_HAL_ERROR_INVALID_INPUT;
    }

    ifx_se_mpc_rot_cfg_crc_t config;
    ifx_se_status_t se_status;
#if IFX_SE_RT_FULL_ADDRESS_MPC_API
    /* Newer SE RT services take the region's absolute secure address
     * (memory base + offset), so more than one external memory can be
     * configured. */
    config.mpc_config.addr_offset = sert_mem->mem->s_address + mpc_reg_cfg->offset;
#else /* IFX_SE_RT_FULL_ADDRESS_MPC_API */
    /* Legacy SE RT services can configure only a single external memory and
     * take the offset within that memory. */
    config.mpc_config.addr_offset = mpc_reg_cfg->offset;
#endif /* IFX_SE_RT_FULL_ADDRESS_MPC_API */
    config.mpc_config.size = mpc_reg_cfg->size;
    /* cy_en_mpc_size_t and ifx_se_mpc_size_t share the same numeric encoding
     * (value == log2(block_bytes) - 5), so the block size maps by a direct cast. */
    config.mpc_config.region_size = (ifx_se_mpc_size_t)mpc_reg_cfg->mpc_block_size;

    /* MPC config works with PC >= IFX_PC_TFM_SPM_ID. Lower PCs
     * are responsible for protecting/verifying their own resources. */
    for (uint32_t pc = IFX_PC_TFM_SPM_ID; pc < MPC_PC_NR; pc++) {
        /* Apply only the PCs explicitly requested by the configuration and
         * leave any other PC settings unchanged. */
        if (IFX_GET_PC(mpc_reg_cfg->pc_apply_mask, pc) == 0U) {
            continue;
        }

        config.mpc_config.pc = (ifx_se_mpc_prot_context_t)pc;
        config.mpc_config.secure = IFX_SE_MPC_SECURE;
        config.mpc_config.access = IFX_SE_MPC_ACCESS_DISABLED;

        if (IFX_GET_PC(mpc_reg_cfg->ns_mask, pc) != 0U) {
            config.mpc_config.secure = IFX_SE_MPC_NON_SECURE;
        } else {
            config.mpc_config.secure = IFX_SE_MPC_SECURE;
        }

        uint8_t r_mask = IFX_GET_PC(mpc_reg_cfg->r_mask, pc);
        uint8_t w_mask = IFX_GET_PC(mpc_reg_cfg->w_mask, pc);
        if ((r_mask != 0U) && (w_mask != 0U)) {
            config.mpc_config.access = IFX_SE_MPC_ACCESS_RW;
        } else if (r_mask != 0U) {
            config.mpc_config.access = IFX_SE_MPC_ACCESS_R;
        } else if (w_mask != 0U) {
            config.mpc_config.access = IFX_SE_MPC_ACCESS_W;
        } else {
            config.mpc_config.access = IFX_SE_MPC_ACCESS_DISABLED;
        }

        config.crc = IFX_CRC32_CALC((uint8_t*)&config.mpc_config, sizeof(config.mpc_config));
        se_status = ifx_se_mpc_config_rot_mpc_struct(&config, IFX_SE_TFM_SYSCALL_CONTEXT);
        if (!IFX_SE_IS_STATUS_SUCCESS(se_status)) {
            return TFM_HAL_ERROR_GENERIC;
        }
    }

    /* Update the descriptor's attribute cache to mirror the applied config. */
    for (uint32_t block_id = info.first_block_idx; block_id < info.last_block_idx; block_id++) {
        /* Keep cached bits for PCs outside info.pc_cache_mask and replace only
         * the selected PC fields with the new attributes from info.attr. */
        /* Read current cached attributes */
        uint32_t cached_attr = sert_mem->attr_cache[block_id];
        /* Zero out bits for PCs being updated, but keep other PC bits unchanged. */
        uint32_t preserved_attr = cached_attr & ~info.pc_cache_mask;
        /* Set bits for PCs being updated according to new configuration. */
        uint32_t updated_attr = preserved_attr | info.attr;

        /* Write updated block to cache */
        sert_mem->attr_cache[block_id] = updated_attr;

#ifdef TFM_FIH_PROFILE_ON
        /* Maintain the complement mirror used to detect cache corruption. */
        sert_mem->attr_cache_inv[block_id] = ~updated_attr;
#endif /* TFM_FIH_PROFILE_ON */
    }

    return TFM_HAL_SUCCESS;
}

#ifdef TFM_FIH_PROFILE_ON
FIH_RET_TYPE(enum tfm_hal_status_t) ifx_mpc_sert_verify_configuration(
                                        const ifx_mpc_raw_region_config_t* mpc_reg_cfg)
{
    /* We can't read the MPC directly because the PPCs restrict access to PC0/1 only
     * and the SE RT doesn't provide a way to read it
     * But it's possible to validate the MPC attribute cache */
    const ifx_mpc_sert_mem_t *sert_mem = ifx_mpc_sert_find_mem(mpc_reg_cfg->mpc_base);
    if (sert_mem == NULL) {
        FIH_RET(TFM_HAL_ERROR_GENERIC);
    }

    ifx_mpc_ext_cache_cfg_info_t info;
    if (ifx_mpc_sert_config_to_cache_info(sert_mem, mpc_reg_cfg, &info) != TFM_HAL_SUCCESS) {
        FIH_RET(TFM_HAL_ERROR_GENERIC);
    }

    for (uint32_t block_id = info.first_block_idx; block_id < info.last_block_idx; block_id++) {
        /* Verify only the PCs explicitly requested by the configuration and
         * leave any other PC settings unchanged. */
        uint32_t selected_attr = sert_mem->attr_cache[block_id] & info.pc_cache_mask;

        if (selected_attr != info.attr) {
            FIH_RET(TFM_HAL_ERROR_GENERIC);
        }
    }
    FIH_RET(TFM_HAL_SUCCESS);
}
#endif /* TFM_FIH_PROFILE_ON */

FIH_RET_TYPE(enum tfm_hal_status_t) ifx_mpc_sert_memory_check(
                                           const struct ifx_partition_info_t *p_info,
                                           const ifx_memory_config_t* memory_config,
                                           uintptr_t base,
                                           size_t size,
                                           uint32_t access_type)
{
    if (memory_config == NULL) {
        FIH_RET(TFM_HAL_ERROR_MEM_FAULT);
    }

    /* Resolve the external memory (and its attribute cache) served by this MPC. */
    const ifx_mpc_sert_mem_t *sert_mem = ifx_mpc_sert_find_mem(memory_config->mpc);
    if (sert_mem == NULL) {
        FIH_RET(TFM_HAL_ERROR_MEM_FAULT);
    }

    if (memory_config->mpc_block_size != sert_mem->mem->mpc_block_size) {
        FIH_RET(TFM_HAL_ERROR_MEM_FAULT);
    }

    uint32_t block_size      = IFX_MPC_EXT_BLOCK_SIZE_TO_BYTES(memory_config->mpc_block_size);
    uint32_t block_count     = sert_mem->block_count;
    uint32_t offset          = IFX_S_ADDRESS_ALIAS(base) - memory_config->s_address;
    uint32_t first_block_idx = offset / block_size;
    uint32_t last_block_idx  = IFX_ROUND_UP_TO_MULTIPLE(offset + size,
                                                        block_size)
                                / block_size;
    if ((first_block_idx >= block_count) ||
        (last_block_idx > block_count)) {
        FIH_RET(TFM_HAL_ERROR_MEM_FAULT);
    }

    bool is_secure = (access_type & TFM_HAL_ACCESS_NS) == 0U;
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_10_1, "external macro IS_NS_AGENT interprets non-boolean type as a boolean")
    uint32_t pc = (uint32_t)fih_int_decode(IFX_GET_PARTITION_PC(p_info, is_secure));

    /* Only the bits relevant to the requesting PC participate in the check. */
    uint32_t mask = IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_NS_BIT);
    uint32_t attr = is_secure ? 0U : IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_NS_BIT);

    if ((access_type & TFM_HAL_ACCESS_READABLE) != 0U) {
        mask |= IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_READ_BIT);
        attr |= IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_READ_BIT);
    }

    if ((access_type & TFM_HAL_ACCESS_WRITABLE) != 0U) {
        mask |= IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_WRITE_BIT);
        attr |= IFX_MPC_EXT_CACHE_PC_BIT(pc, IFX_MPC_EXT_CACHE_WRITE_BIT);
    }

#if defined(TFM_FIH_PROFILE_ON)
    /* Re-evaluate the decision inputs from a re-read access_type and confirm
     * they match the first pass */
    (void)fih_delay();
    volatile uint32_t access_type_2 = access_type;
    uint32_t offset2          = IFX_S_ADDRESS_ALIAS(base) - memory_config->s_address;
    uint32_t first_block_idx2 = offset2 / block_size;
    uint32_t last_block_idx2  = IFX_ROUND_UP_TO_MULTIPLE(offset2 + size,
                                                         block_size)
                                 / block_size;
    bool is_secure2 = (access_type_2 & TFM_HAL_ACCESS_NS) == 0U;
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_10_1, "external macro IS_NS_AGENT interprets non-boolean type as a boolean")
    uint32_t pc2 = (uint32_t)fih_int_decode(IFX_GET_PARTITION_PC(p_info, is_secure2));
    uint32_t mask2 = IFX_MPC_EXT_CACHE_PC_BIT(pc2, IFX_MPC_EXT_CACHE_NS_BIT);
    uint32_t attr2 = is_secure2 ? 0U : IFX_MPC_EXT_CACHE_PC_BIT(pc2, IFX_MPC_EXT_CACHE_NS_BIT);
    if ((access_type_2 & TFM_HAL_ACCESS_READABLE) != 0U) {
        mask2 |= IFX_MPC_EXT_CACHE_PC_BIT(pc2, IFX_MPC_EXT_CACHE_READ_BIT);
        attr2 |= IFX_MPC_EXT_CACHE_PC_BIT(pc2, IFX_MPC_EXT_CACHE_READ_BIT);
    }
    if ((access_type_2 & TFM_HAL_ACCESS_WRITABLE) != 0U) {
        mask2 |= IFX_MPC_EXT_CACHE_PC_BIT(pc2, IFX_MPC_EXT_CACHE_WRITE_BIT);
        attr2 |= IFX_MPC_EXT_CACHE_PC_BIT(pc2, IFX_MPC_EXT_CACHE_WRITE_BIT);
    }
    (void)fih_delay();
    if ((mask != mask2) || (attr != attr2) ||
        (first_block_idx != first_block_idx2) || (last_block_idx != last_block_idx2)) {
        FIH_RET(TFM_HAL_ERROR_MEM_FAULT);
    }

    volatile uint32_t validated_block_count = 0U;
#endif /* defined(TFM_FIH_PROFILE_ON) */

    for (uint32_t block_id = first_block_idx; block_id < last_block_idx; block_id++) {
        uint32_t cached_attr = sert_mem->attr_cache[block_id];

#if defined(TFM_FIH_PROFILE_ON)
        /* Confirm the word matches its complement mirror to detect corruption. */
        (void)fih_delay();
        if (cached_attr != (uint32_t)~sert_mem->attr_cache_inv[block_id]) {
            FIH_RET(TFM_HAL_ERROR_MEM_FAULT);
        }
        validated_block_count++;
#endif /* defined(TFM_FIH_PROFILE_ON) */

        if ((cached_attr & mask) != attr) {
            FIH_RET(TFM_HAL_ERROR_MEM_FAULT);
        }
    }

#if defined(TFM_FIH_PROFILE_ON)
    /* Confirm every block was actually inspected. */
    (void)fih_delay();
    if (validated_block_count != (last_block_idx - first_block_idx)) {
        FIH_RET(TFM_HAL_ERROR_MEM_FAULT);
    }
#endif /* defined(TFM_FIH_PROFILE_ON) */

    FIH_RET(TFM_HAL_SUCCESS);
}

FIH_RET_TYPE(enum tfm_hal_status_t) ifx_mpc_sert_init(void)
{
#if !IFX_SE_RT_FULL_ADDRESS_MPC_API
    /* Legacy SE RT services can configure only a single external memory, so the
     * platform must describe exactly one. */
    if (ifx_mpc_sert_memories_count != 1U) {
        FIH_RET(TFM_HAL_ERROR_GENERIC);
    }
#endif /* !IFX_SE_RT_FULL_ADDRESS_MPC_API */

    /* Setup the attribute cache of every external memory to its reset defaults.
     * The cache is used to implement ifx_mpc_sert_memory_check(). */
    for (size_t idx = 0U; idx < ifx_mpc_sert_memories_count; idx++) {
        const ifx_mpc_sert_mem_t *sert_mem = &ifx_mpc_sert_memories[idx];
        uint32_t block_count = sert_mem->block_count;

        for (uint32_t block_id = 0U; block_id < block_count; block_id++) {
            sert_mem->attr_cache[block_id] = IFX_MPC_SERT_DEFAULT_ATTR;
#ifdef TFM_FIH_PROFILE_ON
            sert_mem->attr_cache_inv[block_id] = ~(uint32_t)IFX_MPC_SERT_DEFAULT_ATTR;
#endif /* TFM_FIH_PROFILE_ON */
        }
    }

#if defined(TFM_FIH_PROFILE_ON)
    /* Validate the cache used by ifx_mpc_sert_memory_check() */
    (void)fih_delay();
    for (size_t idx = 0U; idx < ifx_mpc_sert_memories_count; idx++) {
        const ifx_mpc_sert_mem_t *sert_mem = &ifx_mpc_sert_memories[idx];
        uint32_t block_count = sert_mem->block_count;

        for (uint32_t block_id = 0U; block_id < block_count; block_id++) {
            if ((sert_mem->attr_cache[block_id] != IFX_MPC_SERT_DEFAULT_ATTR) ||
                (sert_mem->attr_cache[block_id] !=
                 (uint32_t)~sert_mem->attr_cache_inv[block_id])) {
                FIH_RET(TFM_HAL_ERROR_GENERIC);
            }
        }
    }
#endif

    FIH_RET(TFM_HAL_SUCCESS);
}
