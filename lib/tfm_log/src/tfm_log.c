/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "config_tfm.h"
#include "tfm_vprintf_priv.h"
#include "tfm_log.h"
#include "uart_stdout.h"
#include "coverity_check.h"

#ifdef LOG_PRIV_BUFFER_SIZE
struct tfm_log_priv_data {
    uint8_t buf_pos;
    size_t total_output_chars;
    char buf[LOG_PRIV_BUFFER_SIZE];
};

static void output_buf(struct tfm_log_priv_data *data, uint32_t buf_len)
{
    int32_t ret;

    ret = stdio_output_string(data->buf, buf_len);
    if (ret > 0) {
        data->total_output_chars += ret;
    }
}

static void output_string_to_buf(void *priv, const char *str, uint32_t len)
{
    TFM_COVERITY_DEVIATE_LINE(MISRA_C_2023_Rule_11_5, "Intentional pointer cast")
    struct tfm_log_priv_data *data = (struct tfm_log_priv_data *)priv;

    if ((data->buf_pos + len) > LOG_PRIV_BUFFER_SIZE) {
        /* Flush current buffer and re-use */
        output_buf(data, data->buf_pos);
        data->buf_pos = 0;

        /* Handle strings larger than buffer with multiple flushes */
        for (; len > LOG_PRIV_BUFFER_SIZE;
             len -= LOG_PRIV_BUFFER_SIZE, str += LOG_PRIV_BUFFER_SIZE) {
            memcpy(data->buf, str, LOG_PRIV_BUFFER_SIZE);
            output_buf(data, LOG_PRIV_BUFFER_SIZE);
        }
    }

    memcpy(data->buf + data->buf_pos, str, len);
    data->buf_pos += len;
}

void tfm_log(const char *fmt, ...)
{
    va_list args;
    struct tfm_log_priv_data data;

    data.buf_pos = 0;
    data.total_output_chars = 0;

    va_start(args, fmt);
    tfm_vprintf(output_string_to_buf, &data, fmt, args, true);
    va_end(args);

    output_buf(&data, data.buf_pos);
}
#else /* LOG_PRIV_BUFFER_SIZE */
static void output_log(void *priv, const char *str, uint32_t len)
{
    stdio_output_string(str, len);
}

void tfm_log(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    tfm_vprintf(output_log, NULL, fmt, args, true);
    va_end(args);
}
#endif /* LOG_PRIV_BUFFER_SIZE */
