/*
 * (c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* The platform provides crypto_keys_rram.h describing the built-in keys (IAK
 * private, IAK public, HUK). For each key it defines the RRAM address and the
 * stored size via IFX_CRYPTO_KEYS_<KEY>_ADDR and
 * IFX_CRYPTO_KEYS_<KEY>_STORED_SIZE, plus its metadata (KEY_BITS / KEY_LEN /
 * ALGORITHM / TYPE). Whether the keys sit in a single contiguous block or at
 * independent addresses is the platform's concern; this file only reads the
 * addresses and sizes it is given.
 *
 * \ref ifx_assets_rram_read_asset reads and validates each key, expecting the
 * key material to be immediately followed by its 4-byte CRC32.
 */

#include <string.h>
#include "crypto_keys_rram.h"
#include "ifx_assets_rram.h"
#include "mbedtls/platform_util.h"
#include "tfm_builtin_key_ids.h"
#include "tfm_builtin_key_loader.h"
#include "tfm_plat_crypto_keys.h"
#include "coverity_check.h"

#ifdef TFM_PARTITION_INITIAL_ATTESTATION
#include "psa_manifest/pid.h"
#endif /* TFM_PARTITION_INITIAL_ATTESTATION */

#define NUMBER_OF_ELEMENTS_OF(x) sizeof(x)/sizeof(*x)

#define MAPPED_TZ_NS_AGENT_DEFAULT_CLIENT_ID      0xaaaa7fffU
#define MAPPED_MAILBOX_NS_AGENT_DEFAULT_CLIENT_ID 0xaaaaffffU
#define TFM_TZ_NS_PARTITION_ID                    MAPPED_TZ_NS_AGENT_DEFAULT_CLIENT_ID
#define TFM_MBOX_NS_PARTITION_ID                  MAPPED_MAILBOX_NS_AGENT_DEFAULT_CLIENT_ID

static enum tfm_plat_err_t ifx_plat_get_huk(const void *ctx,
                                            uint8_t *buf, size_t buf_len,
                                            size_t *key_len,
                                            psa_key_bits_t *key_bits,
                                            psa_algorithm_t *algorithm,
                                            psa_key_type_t *type)
{
    size_t my_key_len = IFX_CRYPTO_KEYS_HUK_STORED_SIZE;
    (void)ctx;

    if (buf_len < my_key_len) {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }

#ifdef IFX_CRYPTO_KEYS_MOCK_HUK_VALUE
    /* IMPROVEMENT: (MXSV2BOOT-5196) the HUK is not yet provisioned in
     * hardware, so its region is empty. Return the fixed key
     * IFX_CRYPTO_KEYS_MOCK_HUK_VALUE provided by the platform's
     * crypto_keys_rram.h. Remove this block once the platform stops
     * defining IFX_CRYPTO_KEYS_MOCK_HUK_VALUE to restore RRAM reads. */
    static const uint8_t mock_huk[] = IFX_CRYPTO_KEYS_MOCK_HUK_VALUE;
    if (sizeof(mock_huk) < my_key_len) {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }
    (void)memcpy(buf, mock_huk, my_key_len);
#else /* IFX_CRYPTO_KEYS_MOCK_HUK_VALUE */
    int32_t err = ifx_assets_rram_read_asset(IFX_CRYPTO_KEYS_HUK_ADDR, buf, my_key_len);
    if (err != ARM_DRIVER_OK) {
        /* buf may already hold key bytes read before the integrity check failed. */
        mbedtls_platform_zeroize(buf, my_key_len);
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }
#endif /* IFX_CRYPTO_KEYS_MOCK_HUK_VALUE */

    *key_bits = IFX_CRYPTO_KEYS_HUK_KEY_BITS;
    *key_len = IFX_CRYPTO_KEYS_HUK_KEY_LEN;
    *algorithm = IFX_CRYPTO_KEYS_HUK_ALGORITHM;
    *type = IFX_CRYPTO_KEYS_HUK_TYPE;

    return TFM_PLAT_ERR_SUCCESS;
}

#ifdef TFM_PARTITION_INITIAL_ATTESTATION
static enum tfm_plat_err_t ifx_plat_get_iak_priv(const void *ctx,
                                                 uint8_t *buf, size_t buf_len,
                                                 size_t *key_len,
                                                 psa_key_bits_t *key_bits,
                                                 psa_algorithm_t *algorithm,
                                                 psa_key_type_t *type)
{
    int32_t err;
    size_t my_key_len = IFX_CRYPTO_KEYS_IAK_PRIVATE_STORED_SIZE;
    (void)ctx;

    if (buf_len < my_key_len) {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }

    err = ifx_assets_rram_read_asset(IFX_CRYPTO_KEYS_IAK_PRIVATE_ADDR, buf, my_key_len);
    if (err != ARM_DRIVER_OK) {
        /* buf may already hold key bytes read before the integrity check failed. */
        mbedtls_platform_zeroize(buf, my_key_len);
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    *key_bits = IFX_CRYPTO_KEYS_IAK_PRIVATE_KEY_BITS;
    *key_len = IFX_CRYPTO_KEYS_IAK_PRIVATE_KEY_LEN;
    *algorithm = IFX_CRYPTO_KEYS_IAK_PRIVATE_ALGORITHM;
    *type = IFX_CRYPTO_KEYS_IAK_PRIVATE_TYPE;

    return TFM_PLAT_ERR_SUCCESS;
}

static enum tfm_plat_err_t ifx_plat_get_iak_pub(const void *ctx,
                                                uint8_t *buf, size_t buf_len,
                                                size_t *key_len,
                                                psa_key_bits_t *key_bits,
                                                psa_algorithm_t *algorithm,
                                                psa_key_type_t *type)
{
    int32_t err;
    size_t my_key_len = IFX_CRYPTO_KEYS_IAK_PUBLIC_STORED_SIZE;
    (void)ctx;

    if (buf_len < my_key_len) {
        return TFM_PLAT_ERR_INVALID_INPUT;
    }

    err = ifx_assets_rram_read_asset(IFX_CRYPTO_KEYS_IAK_PUBLIC_ADDR, buf, my_key_len);
    if (err != ARM_DRIVER_OK) {
        /* buf may already hold key bytes read before the integrity check failed. */
        mbedtls_platform_zeroize(buf, my_key_len);
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    *key_bits = IFX_CRYPTO_KEYS_IAK_PUBLIC_KEY_BITS;
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_12_1, "PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE() defined externally, cannot change it")
    *key_len = IFX_CRYPTO_KEYS_IAK_PUBLIC_KEY_LEN;
    *algorithm = IFX_CRYPTO_KEYS_IAK_PUBLIC_ALGORITHM;
    *type = IFX_CRYPTO_KEYS_IAK_PUBLIC_TYPE;

    return TFM_PLAT_ERR_SUCCESS;
}

/**
 * @brief Table describing per-user key policy for the IAK
 *
 */
static const tfm_plat_builtin_key_per_user_policy_t g_iak_per_user_policy[] = {
    { .user = TFM_SP_INITIAL_ATTESTATION, .usage = PSA_KEY_USAGE_SIGN_HASH },
#ifdef TEST_S_ATTESTATION
    { .user = TFM_SP_SECURE_TEST_PARTITION, .usage = PSA_KEY_USAGE_VERIFY_HASH },
#endif /* TEST_S_ATTESTATION */
#ifdef TEST_NS_ATTESTATION
    { .user = TFM_TZ_NS_PARTITION_ID, .usage = PSA_KEY_USAGE_VERIFY_HASH },
    { .user = TFM_MBOX_NS_PARTITION_ID, .usage = PSA_KEY_USAGE_VERIFY_HASH },
#endif /* TEST_NS_ATTESTATION */
};
#endif /* TFM_PARTITION_INITIAL_ATTESTATION */

/**
 * @brief Table describing per-key user policies
 *
 */
static const tfm_plat_builtin_key_policy_t g_builtin_keys_policy[] = {
    { .key_id = TFM_BUILTIN_KEY_ID_HUK,
      .per_user_policy = 0,
      .usage = PSA_KEY_USAGE_DERIVE },
#ifdef TFM_PARTITION_INITIAL_ATTESTATION
    { .key_id = TFM_BUILTIN_KEY_ID_ATTEST_PRIV,
      .per_user_policy = NUMBER_OF_ELEMENTS_OF(g_iak_per_user_policy),
      .policy_ptr = g_iak_per_user_policy },
    { .key_id = TFM_BUILTIN_KEY_ID_ATTEST_PUB,
      .per_user_policy = 0,
      .usage = PSA_KEY_USAGE_VERIFY_HASH },
#endif /* TFM_PARTITION_INITIAL_ATTESTATION */
};

/**
 * @brief Table describing the builtin-in keys (platform keys) available in the platform. Note
 *        that to bind the keys to the tfm_builtin_key_loader driver, the lifetime must be
 *        explicitly set to the one associated to the driver, i.e. TFM_BUILTIN_KEY_LOADER_LIFETIME
 */
TFM_COVERITY_DEVIATE_BLOCK(MISRA_C_2023_Rule_12_1, "PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION() defined externally, cannot change it")
static const tfm_plat_builtin_key_descriptor_t g_builtin_keys_desc[] = {
    { .key_id = TFM_BUILTIN_KEY_ID_HUK,
      .slot_number = TFM_BUILTIN_KEY_SLOT_HUK,
      .lifetime = TFM_BUILTIN_KEY_LOADER_LIFETIME,
      .loader_key_func = ifx_plat_get_huk,
      .loader_key_ctx = NULL,
    },
#ifdef TFM_PARTITION_INITIAL_ATTESTATION
    { .key_id = TFM_BUILTIN_KEY_ID_ATTEST_PRIV,
      .slot_number = TFM_BUILTIN_KEY_SLOT_ATTEST_PRIV,
      .lifetime = TFM_BUILTIN_KEY_LOADER_LIFETIME,
      .loader_key_func = ifx_plat_get_iak_priv,
      .loader_key_ctx = NULL,
    },
    { .key_id = TFM_BUILTIN_KEY_ID_ATTEST_PUB,
      .slot_number = TFM_BUILTIN_KEY_SLOT_ATTEST_PUB,
      .lifetime = TFM_BUILTIN_KEY_LOADER_LIFETIME,
      .loader_key_func = ifx_plat_get_iak_pub,
      .loader_key_ctx = NULL,
    },
#endif /* TFM_PARTITION_INITIAL_ATTESTATION */
};
TFM_COVERITY_BLOCK_END(MISRA_C_2023_Rule_12_1)

size_t tfm_plat_builtin_key_get_policy_table_ptr(const tfm_plat_builtin_key_policy_t *desc_ptr[])
{
    *desc_ptr = &g_builtin_keys_policy[0];
    return NUMBER_OF_ELEMENTS_OF(g_builtin_keys_policy);
}

size_t tfm_plat_builtin_key_get_desc_table_ptr(const tfm_plat_builtin_key_descriptor_t *desc_ptr[])
{
    *desc_ptr = &g_builtin_keys_desc[0];
    return NUMBER_OF_ELEMENTS_OF(g_builtin_keys_desc);
}
