#include "pms9103m.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "PMS9103M";
static pms9103m_config_t s_config;

static esp_err_t pms9103m_build_command(uint8_t cmd, uint8_t data_high, uint8_t data_low,
                                        uint8_t *out_frame, size_t out_frame_size)
{
    if (out_frame_size < 7) {
        return ESP_ERR_INVALID_SIZE;
    }

    out_frame[0] = 0x42;
    out_frame[1] = 0x4d;
    out_frame[2] = cmd;
    out_frame[3] = data_high;
    out_frame[4] = data_low;

    uint16_t checksum = 0;
    for (int i = 0; i < 5; i++) {
        checksum += out_frame[i];
    }

    out_frame[5] = (checksum >> 8) & 0xff;
    out_frame[6] = checksum & 0xff;
    return ESP_OK;
}

static esp_err_t pms9103m_parse_frame(const uint8_t *frame, pms9103m_data_t *data)
{
    if (frame == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t length = (frame[2] << 8) | frame[3];
    if (length != 28) {
        return ESP_ERR_INVALID_SIZE;
    }

    uint16_t checksum = (frame[30] << 8) | frame[31];
    uint16_t sum = 0;
    for (int i = 0; i < 30; i++) {
        sum += frame[i];
    }
    if (sum != checksum) {
        ESP_LOGW(TAG, "Checksum mismatch: expected 0x%04x computed 0x%04x", checksum, sum);
        return ESP_ERR_INVALID_CRC;
    }

    data->pm1_0_cf1 = (frame[4] << 8) | frame[5];
    data->pm2_5_cf1 = (frame[6] << 8) | frame[7];
    data->pm10_cf1 = (frame[8] << 8) | frame[9];
    data->pm1_0_atm = (frame[10] << 8) | frame[11];
    data->pm2_5_atm = (frame[12] << 8) | frame[13];
    data->pm10_atm = (frame[14] << 8) | frame[15];
    data->count_0_3um = (frame[16] << 8) | frame[17];
    data->count_0_5um = (frame[18] << 8) | frame[19];
    data->count_1_0um = (frame[20] << 8) | frame[21];
    data->count_2_5um = (frame[22] << 8) | frame[23];
    data->count_5_0um = (frame[24] << 8) | frame[25];
    data->count_10_0um = (frame[26] << 8) | frame[27];
    data->version = frame[28];
    data->error_code = frame[29];

    return ESP_OK;
}

static esp_err_t pms9103m_read_bytes(uint8_t *buf, size_t len, TickType_t ticks_to_wait)
{
    if (buf == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t received = 0;
    while (received < len) {
        int ret = uart_read_bytes(s_config.uart_num, buf + received, len - received, ticks_to_wait);
        if (ret < 0) {
            return ESP_FAIL;
        }
        if (ret == 0) {
            return ESP_ERR_TIMEOUT;
        }
        received += ret;
    }
    return ESP_OK;
}

esp_err_t pms9103m_init(const pms9103m_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_config = *config;

    uart_config_t uart_config = {
        .baud_rate = s_config.baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(s_config.uart_num, 1024, 1024, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_param_config(s_config.uart_num, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_set_pin(s_config.uart_num, s_config.tx_io_num, s_config.rx_io_num,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t pms9103m_read_frame(pms9103m_data_t *data, TickType_t ticks_to_wait)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t frame[32];
    memset(frame, 0, sizeof(frame));

    while (true) {
        uint8_t header;
        esp_err_t err = pms9103m_read_bytes(&header, 1, ticks_to_wait);
        if (err != ESP_OK) {
            return err;
        }
        if (header != 0x42) {
            continue;
        }

        err = pms9103m_read_bytes(frame + 1, 1, ticks_to_wait);
        if (err != ESP_OK) {
            continue;
        }
        if (frame[1] != 0x4d) {
            if (frame[1] == 0x42) {
                frame[0] = 0x42;
                continue;
            }
            continue;
        }

        frame[0] = 0x42;
        err = pms9103m_read_bytes(frame + 2, 2, ticks_to_wait);
        if (err != ESP_OK) {
            continue;
        }

        uint16_t length = (frame[2] << 8) | frame[3];
        if (length != 28) {
            ESP_LOGW(TAG, "Invalid frame length: %u", length);
            continue;
        }

        err = pms9103m_read_bytes(frame + 4, 28, ticks_to_wait);
        if (err != ESP_OK) {
            continue;
        }

        return pms9103m_parse_frame(frame, data);
    }
}

static esp_err_t pms9103m_send_command(uint8_t cmd, uint8_t data_high, uint8_t data_low)
{
    uint8_t frame[7];
    esp_err_t err = pms9103m_build_command(cmd, data_high, data_low, frame, sizeof(frame));
    if (err != ESP_OK) {
        return err;
    }

    int written = uart_write_bytes(s_config.uart_num, (const char *)frame, sizeof(frame));
    if (written != sizeof(frame)) {
        ESP_LOGE(TAG, "Failed to write command frame");
        return ESP_FAIL;
    }

    return uart_wait_tx_done(s_config.uart_num, pdMS_TO_TICKS(1000));
}

esp_err_t pms9103m_request_passive_data(pms9103m_data_t *data, TickType_t ticks_to_wait)
{
    esp_err_t err = pms9103m_send_command(0xe2, 0x00, 0x00);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send passive request: %s", esp_err_to_name(err));
        return err;
    }

    return pms9103m_read_frame(data, ticks_to_wait);
}

esp_err_t pms9103m_set_mode(pms9103m_mode_t mode, TickType_t ticks_to_wait)
{
    uint8_t mode_byte = (mode == PMS9103M_MODE_ACTIVE) ? 0x01 : 0x00;
    esp_err_t err = pms9103m_send_command(0xe1, 0x00, mode_byte);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send mode command: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}
