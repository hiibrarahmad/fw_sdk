/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "adaptation.h"
#include <string.h>
#include <stdlib.h>
#include "net.h"
#include "access.h"
#include "transport.h"
#include "api/properties.h"
#include "sensor.h"

#if (CONFIG_BT_MESH_SENSOR_SRV || CONFIG_BT_MESH_SENSOR_CLI)

#define LOG_TAG             "[Mesh-sensor]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

/** Scale away the sensor_value fraction, to allow integer math */
#define SENSOR_MILL(_val) ((1000000LL * (_val)->val1) + (_val)->val2)

static enum bt_mesh_sensor_cadence
sensor_cadence(const struct bt_mesh_sensor_threshold *threshold,
               const struct sensor_value *curr) {
    int64_t high_mill = SENSOR_MILL(&threshold->range.high);
    int64_t low_mill = SENSOR_MILL(&threshold->range.low);

    if (high_mill == low_mill)
    {
        return BT_MESH_SENSOR_CADENCE_NORMAL;
    }

    int64_t curr_mill = SENSOR_MILL(curr);
    bool in_range = (curr_mill >= MIN(low_mill, high_mill) &&
    curr_mill <= MAX(low_mill, high_mill));

    return in_range ? threshold->range.cadence : !threshold->range.cadence;
}

bool bt_mesh_sensor_delta_threshold(const struct bt_mesh_sensor *sensor,
                                    const struct sensor_value *curr)
{
    struct sensor_value delta = {
        curr->val1 - sensor->state.prev.val1,
        curr->val2 - sensor->state.prev.val2,
    };
    int64_t delta_mill = SENSOR_MILL(&delta);
    int64_t thrsh_mill;

    if (delta_mill < 0) {
        delta_mill = -delta_mill;
        thrsh_mill = SENSOR_MILL(&sensor->state.threshold.delta.down);
    } else {
        thrsh_mill = SENSOR_MILL(&sensor->state.threshold.delta.up);
    }

    /* If the threshold value is a perentage, we should calculate the actual
     * threshold value relative to the previous value.
     */
    if (sensor->state.threshold.delta.type ==
        BT_MESH_SENSOR_DELTA_PERCENT) {
        int64_t prev_mill = llabs(SENSOR_MILL(&sensor->state.prev));

        thrsh_mill = (prev_mill * thrsh_mill) / (100LL * 1000000LL);
    }

    if (IS_ENABLED(CONFIG_BT_MESH_MODEL_LOG_LEVEL_DBG)) {
        char delta_str[BT_MESH_SENSOR_CH_STR_LEN];
        char curr_str[BT_MESH_SENSOR_CH_STR_LEN];
        char prev_str[BT_MESH_SENSOR_CH_STR_LEN];
        char thrsh_str[BT_MESH_SENSOR_CH_STR_LEN];

        strcpy(delta_str, bt_mesh_sensor_ch_str(&delta));
        strcpy(curr_str, bt_mesh_sensor_ch_str(curr));
        strcpy(prev_str, bt_mesh_sensor_ch_str(&sensor->state.prev));
        strcpy(thrsh_str,
               delta_mill < 0 ? bt_mesh_sensor_ch_str(&sensor->state.threshold.delta.down) :
               bt_mesh_sensor_ch_str(&sensor->state.threshold.delta.up));

        log_debug("Delta: %s (%s - %s) thrsh: %s", delta_str, curr_str, prev_str, thrsh_str);
    }

    return (delta_mill > thrsh_mill);
}

void bt_mesh_sensor_cadence_set(struct bt_mesh_sensor *sensor,
                                enum bt_mesh_sensor_cadence cadence)
{
    sensor->state.fast_pub = (cadence == BT_MESH_SENSOR_CADENCE_FAST);
}

int sensor_status_id_encode(struct net_buf_simple *buf, uint8_t len, uint16_t id)
{
    if ((len > 0 && len <= 16) && id < 2048) {
        if (net_buf_simple_tailroom(buf) < 2 + len) {
            return -ENOMEM;
        }
        net_buf_simple_add_le16(buf, ((len - 1) << 1) | (id << 5));
        log_debug_hexdump(buf->data, buf->len);
    } else {
        if (net_buf_simple_tailroom(buf) < 3 + len) {
            return -ENOMEM;
        }
        net_buf_simple_add_u8(buf, BIT(0) | (((len - 1) & BIT_MASK(7))
                                             << 1));
        net_buf_simple_add_le16(buf, id);
    }

    return 0;
}

void sensor_status_id_decode(struct net_buf_simple *buf, uint8_t *len, uint16_t *id)
{
    uint8_t first = net_buf_simple_pull_u8(buf);
    if (first & BIT(0)) { /* long format */
        if (buf->len < 2) {
            *len = 0;
            *id = BT_MESH_PROP_ID_PROHIBITED;
            return;
        }
        *len = ((first >> 1) + 1) & 0x7f;
        *id = net_buf_simple_pull_le16(buf);
    } else if (buf->len < 1) {
        *len = 0;
        *id = BT_MESH_PROP_ID_PROHIBITED;
    } else {
        *len = ((first >> 1) & BIT_MASK(4)) + 1;
        *id = (first >> 5) | (net_buf_simple_pull_u8(buf) << 3);
    }
}

static void tolerance_decode(uint16_t encoded, struct sensor_value *tolerance)
{
    uint32_t toll_mill = (encoded * 100ULL * 1000000ULL) / 4095ULL;

    tolerance->val1 = toll_mill / 1000000ULL;
    tolerance->val2 = toll_mill % 1000000ULL;
}
static uint16_t tolerance_encode(const struct sensor_value *tol)
{
    uint64_t tol_mill = 1000000ULL * tol->val1 + tol->val2;

    if (tol_mill > (1000000ULL * 100ULL)) {
        return 0;
    }
    return (tol_mill * 4095ULL + (1000000ULL * 50ULL)) / (1000000ULL * 100ULL);
}

void sensor_descriptor_decode(struct net_buf_simple *buf,
                              struct bt_mesh_sensor_info *sensor)
{
    uint32_t tolerances;

    sensor->id = net_buf_simple_pull_le16(buf);
    tolerances = net_buf_simple_pull_le24(buf);
    tolerance_decode(tolerances & BIT_MASK(12),
                     &sensor->descriptor.tolerance.positive);
    tolerance_decode(tolerances >> 12,
                     &sensor->descriptor.tolerance.negative);
    sensor->descriptor.sampling_type = net_buf_simple_pull_u8(buf);
    sensor->descriptor.period =
        sensor_powtime_decode(net_buf_simple_pull_u8(buf));
    sensor->descriptor.update_interval =
        sensor_powtime_decode(net_buf_simple_pull_u8(buf));
}

void sensor_descriptor_encode(struct net_buf_simple *buf,
                              struct bt_mesh_sensor *sensor)
{
    net_buf_simple_add_le16(buf, sensor->type->id);

    const struct bt_mesh_sensor_descriptor dummy = { 0 };
    const struct bt_mesh_sensor_descriptor *d =
            sensor->descriptor ? sensor->descriptor : &dummy;

    uint16_t tol_pos = tolerance_encode(&d->tolerance.positive);
    uint16_t tol_neg = tolerance_encode(&d->tolerance.negative);

    net_buf_simple_add_u8(buf, tol_pos & 0xff);
    net_buf_simple_add_u8(buf,
                          ((tol_pos >> 8) & BIT_MASK(4)) | (tol_neg << 4));
    net_buf_simple_add_u8(buf, tol_neg >> 4);
    net_buf_simple_add_u8(buf, d->sampling_type);

    net_buf_simple_add_u8(buf, sensor_powtime_encode(d->period));
    net_buf_simple_add_u8(buf, sensor_powtime_encode(d->update_interval));
}

int sensor_value_encode(struct net_buf_simple *buf,
                        const struct bt_mesh_sensor_type *type,
                        const struct sensor_value *values)
{
    /* The API assumes that `values` array size is always CONFIG_BT_MESH_SENSOR_CHANNELS_MAX. */
    __ASSERT_NO_MSG(type->channel_count <= CONFIG_BT_MESH_SENSOR_CHANNELS_MAX);

    for (uint32_t i = 0; i < type->channel_count; ++i) {
        int err;

        err = sensor_ch_encode(buf, type->channels[i].format,
                               &values[i]);
        if (err) {
            return err;
        }
    }

    return 0;
}

int sensor_value_decode(struct net_buf_simple *buf,
                        const struct bt_mesh_sensor_type *type,
                        struct sensor_value *values)
{
    int err;

    /* The API assumes that `values` array size is always CONFIG_BT_MESH_SENSOR_CHANNELS_MAX. */
    __ASSERT_NO_MSG(type->channel_count <= CONFIG_BT_MESH_SENSOR_CHANNELS_MAX);

    for (uint32_t i = 0; i < type->channel_count; ++i) {
        err = sensor_ch_decode(buf, type->channels[i].format,
                               &values[i]);
        if (err) {
            return err;
        }
    }

    return 0;
}

int sensor_ch_encode(struct net_buf_simple *buf,
                     const struct bt_mesh_sensor_format *format,
                     const struct sensor_value *value)
{
    return format->encode(format, value, buf);
}

int sensor_ch_decode(struct net_buf_simple *buf,
                     const struct bt_mesh_sensor_format *format,
                     struct sensor_value *value)
{
    return format->decode(format, buf, value);
}

int sensor_status_encode(struct net_buf_simple *buf,
                         const struct bt_mesh_sensor *sensor,
                         const struct sensor_value *values)
{
    const struct bt_mesh_sensor_type *type = sensor->type;
    size_t size = 0;
    int err;

    for (uint32_t i = 0; i < type->channel_count; ++i) {
        size += type->channels[i].format->size;
    }
    err = sensor_status_id_encode(buf, size, type->id);
    if (err) {
        return err;
    }

    return sensor_value_encode(buf, type, values);
}

const struct bt_mesh_sensor_format *
bt_mesh_sensor_column_format_get(const struct bt_mesh_sensor_type *type)
{
    if (type->channel_count > 2) {
        return type->channels[1].format;
    }

    return NULL;
}

int sensor_column_value_encode(struct net_buf_simple *buf,
                               struct bt_mesh_sensor_srv *srv,
                               struct bt_mesh_sensor *sensor,
                               struct bt_mesh_msg_ctx *ctx,
                               uint32_t column_index)
{
    struct sensor_value values[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
    int err = sensor->series.get(srv, sensor, ctx, column_index, values);

    if (err) {
        return err;
    }

    return sensor_value_encode(buf, sensor->type, values);
}

int sensor_column_encode(struct net_buf_simple *buf,
                         struct bt_mesh_sensor_srv *srv,
                         struct bt_mesh_sensor *sensor,
                         struct bt_mesh_msg_ctx *ctx,
                         const struct bt_mesh_sensor_column *col)
{
    int col_index = col - sensor->series.columns;

    const struct bt_mesh_sensor_format *col_format;
    const uint64_t width_million =
        (col->end.val1 - col->start.val1) * 1000000ULL +
        (col->end.val2 - col->start.val2);
    const struct sensor_value width = {
        .val1 = width_million / 1000000ULL,
        .val2 = width_million % 1000000ULL,
    };
    int err;

    log_debug("Column width: %s", bt_mesh_sensor_ch_str(&width));

    col_format = bt_mesh_sensor_column_format_get(sensor->type);
    if (!col_format) {
        return -ENOTSUP;
    }

    err = sensor_ch_encode(buf, col_format, &col->start);
    if (err) {
        return err;
    }

    /* The sensor columns are transmitted as start+width, not start+end: */
    err = sensor_ch_encode(buf, col_format, &width);
    if (err) {
        return err;
    }

    return sensor_column_value_encode(buf, srv, sensor, ctx, col_index);
}

int sensor_column_decode(
    struct net_buf_simple *buf, const struct bt_mesh_sensor_type *type,
    struct bt_mesh_sensor_column *col,
    struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX])
{
    const struct bt_mesh_sensor_format *col_format;
    int err;

    col_format = bt_mesh_sensor_column_format_get(type);
    if (!col_format) {
        return -ENOTSUP;
    }

    err = sensor_ch_decode(buf, col_format, &col->start);
    if (err) {
        return err;
    }

    if (buf->len == 0) {
        log_error("The requested column didn't exist: %s",
                  bt_mesh_sensor_ch_str(&col->start));
        return -ENOENT;
    }

    struct sensor_value width;

    err = sensor_ch_decode(buf, col_format, &width);
    if (err) {
        return err;
    }

    uint64_t end_mill = (col->start.val1 + width.val1) * 1000000ULL +
                        (col->start.val2 + width.val2);

    col->end.val1 = end_mill / 1000000ULL;
    col->end.val2 = end_mill % 1000000ULL;

    return sensor_value_decode(buf, type, value);
}

uint8_t sensor_value_len(const struct bt_mesh_sensor_type *type)
{
    uint8_t sum = 0;

    for (int i = 0; i < type->channel_count; ++i) {
        sum += type->channels[i].format->size;
    }

    return sum;
}

/* Several timer values in sensors are encoded in the following scheme:
 *
 *     time = pow(1.1 seconds, encoded - 64)
 *
 * To keep the lookup table size and processing time down, this is implemented
 * as a lookup table of the raw value corresponding to every 16th encoded value
 * and a table of multiplicators. The values of the lookup table are
 * pow(1.1, (index * 16) - 64) nanoseconds. The values of the multiplicator
 * array are pow(1.1, index) * 100000. When multiplied by the values in
 * the lookup, the result is pow(1.1, encoded - 64), because of the following
 * property of powers:
 *
 *     pow(A, B + C) = pow(A, B) * pow(A, C)
 *
 * Where in our case, A is 1.1, B is floor((encoded-64), 16) and C is
 * (encoded % 16).
 */
static const uint64_t powtime_lookup[] = {
    2243,	      10307,	    47362,	   217629,
    1000000,      4594972,	    21113776,	   97017233,
    445791568,    2048400214,   9412343651,	   43249464815,
    198730122503, 913159544478, 4195943439113, 19280246755010,
};

static const uint32_t powtime_mul[] = {
    100000, 110000, 121000, 133100, 146410, 161051, 177156, 194871,
    214358, 235794, 259374, 285311, 313842, 345227, 379749, 417724,
};

uint8_t sensor_powtime_encode(uint64_t raw)
{
    if (raw == 0) {
        return 0;
    }

    /* Search through the lookup table to find the highest encoding lower
     * than the raw value.
     */
    uint64_t raw_us = raw * USEC_PER_MSEC;

    if (raw_us < powtime_lookup[0]) {
        return 1;
    }

    const uint64_t *seed = &powtime_lookup[ARRAY_SIZE(powtime_lookup) - 1];

    for (int i = 1; i < ARRAY_SIZE(powtime_lookup); ++i) {
        if (raw_us < powtime_lookup[i]) {
            seed = &powtime_lookup[i - 1];
            break;
        }
    }

    int i;

    for (i = 0; (i < ARRAY_SIZE(powtime_mul) &&
                 raw_us > (*seed * powtime_mul[i]) / 100000);
         i++) {
    }

    uint8_t encoded = ARRAY_SIZE(powtime_mul) * (seed - &powtime_lookup[0]) + i;

    return MIN(encoded, 253);
}

uint64_t sensor_powtime_decode_us(uint8_t val)
{
    if (val == 0) {
        return 0;
    }

    return (powtime_lookup[val / ARRAY_SIZE(powtime_mul)] *
            powtime_mul[val % ARRAY_SIZE(powtime_mul)]) /
           100000L;
}

uint64_t sensor_powtime_decode(uint8_t val)
{
    return sensor_powtime_decode_us(val) / USEC_PER_MSEC;
}

int sensor_cadence_encode(struct net_buf_simple *buf,
                          const struct bt_mesh_sensor_type *sensor_type,
                          uint8_t fast_period_div, uint8_t min_int,
                          const struct bt_mesh_sensor_threshold *threshold)
{
    net_buf_simple_add_u8(buf, ((!!threshold->delta.type) << 7) |
                          (BIT_MASK(7) & fast_period_div));

    // for compiler, not used now.
    // const struct bt_mesh_sensor_format *delta_format =
    // 	(threshold->delta.type == BT_MESH_SENSOR_DELTA_PERCENT) ?
    // 		&bt_mesh_sensor_format_percentage_delta_trigger :
    // 		sensor_type->channels[0].format;
    const struct bt_mesh_sensor_format *delta_format = sensor_type->channels[0].format;
    int err;

    err = sensor_ch_encode(buf, delta_format, &threshold->delta.down);
    if (err) {
        return err;
    }

    err = sensor_ch_encode(buf, delta_format, &threshold->delta.up);
    if (err) {
        return err;
    }

    if (min_int > BT_MESH_SENSOR_INTERVAL_MAX) {
        return -EINVAL;
    }

    net_buf_simple_add_u8(buf, min_int);

    /* Flip the order if the cadence is fast outside. */
    const struct sensor_value *first, *second;

    if (threshold->range.cadence == BT_MESH_SENSOR_CADENCE_FAST) {
        first = &threshold->range.low;
        second = &threshold->range.high;
    } else {
        first = &threshold->range.high;
        second = &threshold->range.low;
    }

    err = sensor_ch_encode(buf, sensor_type->channels[0].format, first);
    if (err) {
        return err;
    }

    return sensor_ch_encode(buf, sensor_type->channels[0].format, second);
}

int sensor_cadence_decode(struct net_buf_simple *buf,
                          const struct bt_mesh_sensor_type *sensor_type,
                          uint8_t *fast_period_div, uint8_t *min_int,
                          struct bt_mesh_sensor_threshold *threshold)
{
    const struct bt_mesh_sensor_format *delta_format;
    uint8_t div_and_type;
    int err;

    div_and_type = net_buf_simple_pull_u8(buf);
    threshold->delta.type = div_and_type >> 7;
    *fast_period_div = div_and_type & BIT_MASK(7);
    if (*fast_period_div > BT_MESH_SENSOR_PERIOD_DIV_MAX) {
        return -EINVAL;
    }

    // for compiler, not used now.
    // delta_format = (threshold->delta.type == BT_MESH_SENSOR_DELTA_PERCENT) ?
    // 		       &bt_mesh_sensor_format_percentage_delta_trigger :
    // 		       sensor_type->channels[0].format;

    delta_format = sensor_type->channels[0].format;

    err = sensor_ch_decode(buf, delta_format, &threshold->delta.down);
    if (err) {
        return err;
    }

    err = sensor_ch_decode(buf, delta_format, &threshold->delta.up);
    if (err) {
        return err;
    }

    *min_int = net_buf_simple_pull_u8(buf);
    if (*min_int > BT_MESH_SENSOR_INTERVAL_MAX) {
        return -EINVAL;
    }

    err = sensor_ch_decode(buf, sensor_type->channels[0].format,
                           &threshold->range.low);
    if (err) {
        return err;
    }

    err = sensor_ch_decode(buf, sensor_type->channels[0].format,
                           &threshold->range.high);
    if (err) {
        return err;
    }

    /* According to the Bluetooth Mesh Model Specification v1.0.1, Section
     * 4.1.3.6, if range.high is lower than range.low, the cadence is fast
     * outside the range. Swap the two in this case.
     */
    if (threshold->range.high.val1 < threshold->range.low.val1 ||
        (threshold->range.high.val1 == threshold->range.low.val1 &&
         threshold->range.high.val2 < threshold->range.low.val2)) {
        struct sensor_value temp;

        temp = threshold->range.high;
        threshold->range.high = threshold->range.low;
        threshold->range.low = temp;

        threshold->range.cadence = BT_MESH_SENSOR_CADENCE_NORMAL;
    } else {
        threshold->range.cadence = BT_MESH_SENSOR_CADENCE_FAST;
    }

    return 0;
}

uint8_t sensor_pub_div_get(const struct bt_mesh_sensor *s, uint32_t base_period)
{
    uint8_t div = s->state.pub_div * s->state.fast_pub;

    while (div != 0 && (base_period >> div) < (1 << s->state.min_int)) {
        div--;
    }

    return div;
}

bool bt_mesh_sensor_value_in_column(const struct sensor_value *value,
                                    const struct bt_mesh_sensor_column *col)
{
    return (value->val1 > col->start.val1 ||
            (value->val1 == col->start.val1 &&
             value->val2 >= col->start.val2)) &&
           (value->val1 < col->end.val1 ||
            (value->val1 == col->end.val1 && value->val2 <= col->end.val2));
}

void sensor_cadence_update(struct bt_mesh_sensor *sensor,
                           const struct sensor_value *value)
{
    enum bt_mesh_sensor_cadence new;

    new = sensor_cadence(&sensor->state.threshold, value);

    /** Use Fast Cadence Period Divisor when publishing when the change exceeds the delta,
     * section 4.3.1.2.4.3, E15551 and E15886.
     */
    if (new == BT_MESH_SENSOR_CADENCE_NORMAL) {
        new = bt_mesh_sensor_delta_threshold(sensor, value) ?
              BT_MESH_SENSOR_CADENCE_FAST : BT_MESH_SENSOR_CADENCE_NORMAL;
    }

    if (sensor->state.fast_pub != new) {
        log_debug("0x%04x new cadence: %s", sensor->type->id,
                  (new == BT_MESH_SENSOR_CADENCE_FAST) ? "fast" :
                  "normal");
    }

    sensor->state.fast_pub = new;
}

const char *bt_mesh_sensor_ch_str(const struct sensor_value *ch)
{
    static char str[BT_MESH_SENSOR_CH_STR_LEN];

    if (bt_mesh_sensor_ch_to_str(ch, str, BT_MESH_SENSOR_CH_STR_LEN) <= 0) {
        strcpy(str, "Unknown format");
    }

    return str;
}

#endif // CONFIG_BT_MESH_SENSOR_SRV