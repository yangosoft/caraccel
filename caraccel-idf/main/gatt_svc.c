/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "gatt_svc.h"
#include "common.h"
#include "heart_rate.h"
#include "sm_time_speed.h"
#include <stdint.h>

#define TAG "GATT_SVC"

/* Private function declarations */
static int fill_measurements_with_latest_data(uint8_t *output_buffer);
static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg);
static int led_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);
static int measurements_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */
/* Measurements service (custom) */
static const ble_uuid128_t measurements_svc_uuid =
    BLE_UUID128_INIT(0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34,
                     0x56, 0x78, 0x9a, 0xbc, 0x00, 0x01);
static const ble_uuid128_t measurements_chr_uuid =
    BLE_UUID128_INIT(0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34,
                     0x56, 0x78, 0x9a, 0xbc, 0x00, 0x02);

// each entry in the array is an uint16_t (2 bytes) *  8 elements
static uint8_t measurements_chr_val[8 * 2] = {0};
static uint16_t measurements_chr_val_handle;
static uint16_t measurements_chr_conn_handle = 0;
static bool measurements_chr_conn_handle_inited = false;
static bool measurements_notify_status = false;

static uint16_t current_measurements_idx = 0;

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /* Measurements service (custom) */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &measurements_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {.uuid = &measurements_chr_uuid.u,
                 .access_cb = measurements_chr_access,
                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                 .val_handle = &measurements_chr_val_handle},
                {0},
            },
    },

    {
        0, /* No more services. */
    },
};

/* Private functions */

static int measurements_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc = 0;

    /* Measurements characteristic is read only (device provides latest array)
     */
    switch (ctxt->op)
    {
    case BLE_GATT_ACCESS_OP_READ_CHR:
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE)
        {
            ESP_LOGI(TAG, "measurements read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        }
        else
        {
            ESP_LOGI(TAG, "measurements read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        if (attr_handle == measurements_chr_val_handle)
        {
            /* Send the current measurements */
            fill_measurements_with_latest_data(measurements_chr_val);

            rc = os_mbuf_append(ctxt->om, &measurements_chr_val,
                                sizeof(measurements_chr_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;

    default:
        goto error;
    }

error:
    ESP_LOGE(TAG,
             "unexpected access operation to measurements characteristic, opcode: %d",
             ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

/* Public functions */
/*void send_heart_rate_indication(void)
{
    if (heart_rate_ind_status && heart_rate_chr_conn_handle_inited)
    {
        ble_gatts_indicate(heart_rate_chr_conn_handle,
                           heart_rate_chr_val_handle);
        ESP_LOGI(TAG, "heart rate indication sent!");
    }
}*/

static int fill_measurements_with_latest_data(uint8_t *output_buffer)
{
    // 0 and 1 for ID
    output_buffer[0] = 3; // ID for historic data
    const struct time_speed_data_t *history;
    uint16_t num_samples = 0;
    sm_time_speed_get_history(&history, &num_samples);
    ESP_LOGI(TAG, "filling measurements with latest data, num_samples in history: %d",
             num_samples);
    if (num_samples > 0)
    {
        // Get the latest sample from history
        // 2 and 3 for current measurement IDX, 4-9 for current measurement data, 10-15 for next measurement data (if any)
        output_buffer[2] = current_measurements_idx & 0xFF; // current measurement IDX (LSB)
        output_buffer[3] = current_measurements_idx >> 8;   // current measurement IDX (MSB)

        output_buffer[4] = history[current_measurements_idx].time_0_80_ms & 0xFF;   // current measurement 0-80ms (LSB)
        output_buffer[5] = history[current_measurements_idx].time_0_80_ms >> 8;     // current measurement 0-80ms (MSB)
        output_buffer[6] = history[current_measurements_idx].time_80_100_ms & 0xFF; // current measurement 80-100ms (LSB)
        output_buffer[7] = history[current_measurements_idx].time_80_100_ms >> 8;   // current measurement 80-100ms (MSB)
        output_buffer[8] = history[current_measurements_idx].time_80_120_ms & 0xFF; // current measurement 80-120ms (LSB)
        output_buffer[9] = history[current_measurements_idx].time_80_120_ms >> 8;   // current

        output_buffer[10] = history[(current_measurements_idx + 1) % num_samples].time_0_80_ms & 0xFF;   // next measurement 0-80ms (LSB)
        output_buffer[11] = history[(current_measurements_idx + 1) % num_samples].time_0_80_ms >> 8;     // next measurement 0-80ms (MSB)
        output_buffer[12] = history[(current_measurements_idx + 1) % num_samples].time_80_100_ms & 0xFF; // next measurement 80-100ms (LSB)
        output_buffer[13] = history[(current_measurements_idx + 1) % num_samples].time_80_100_ms >> 8;   // next measurement 80-100
        output_buffer[14] = history[(current_measurements_idx + 1) % num_samples].time_80_120_ms & 0xFF; // next measurement 80-120ms (LSB)
        output_buffer[15] = history[(current_measurements_idx + 1) % num_samples].time_80_120_ms >> 8;   // next measurement 80-120ms (MSB)

        current_measurements_idx = (current_measurements_idx + 1) % num_samples; // update index for next call
        // Log in hex
        ESP_LOGI(TAG, "output buffer: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                 output_buffer[0], output_buffer[1], output_buffer[2], output_buffer[3],
                 output_buffer[4], output_buffer[5], output_buffer[6], output_buffer[7],
                 output_buffer[8], output_buffer[9], output_buffer[10], output_buffer[11],
                 output_buffer[12], output_buffer[13], output_buffer[14], output_buffer[15]);
    }
    else
    {
        // No samples in history, fill with zeros
        output_buffer[1] = 0; // IDX
        output_buffer[2] = 0;
        output_buffer[3] = 0;
        output_buffer[4] = 0;
        output_buffer[5] = 0;
        output_buffer[6] = 0;
        output_buffer[7] = 0;
    }
    return num_samples;
}

void send_measurements(uint32_t values[8])
{
    // Message payload
    // [0] ID: 0 for speed, 1 for acceleration, 3 historic data
    // [1-7] Data values (e.g. speed in kph * 100, accel in mg, etc.)
    // Historic data: [1] IDX, [2] 0_to_80ms, [3] 80_to_100ms, [4] 80_to_120ms, [5-7] next values for next samples (if any)

    if (values == NULL)
    {
        return;
    }

    // Prepare the message
    uint32_t num_samples = fill_measurements_with_latest_data(measurements_chr_val);

    if (measurements_notify_status && measurements_chr_conn_handle_inited)
    {
        ble_gatts_notify(measurements_chr_conn_handle,
                         measurements_chr_val_handle);
        ESP_LOGI(TAG, "!!!!!!!!!!!measurements for piece %d notification sent!", current_measurements_idx);
    }
}

/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op)
    {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}

/*
 *  GATT server subscribe event callback
 *      1. Update heart rate subscription status
 */

void gatt_svr_subscribe_cb(struct ble_gap_event *event)
{
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE)
    {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    }
    else
    {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }

    /* Check attribute handle */
    if (event->subscribe.attr_handle == measurements_chr_val_handle)
    {
        /* Update measurements subscription status */
        measurements_chr_conn_handle = event->subscribe.conn_handle;
        measurements_chr_conn_handle_inited = true;
        measurements_notify_status = event->subscribe.cur_notify;
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void)
{
    /* Local variables */
    int rc = 0;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}
