/*
* ESP32 CSI Receiver/AP
* This code sets the ESP32 as an AP and acts as a receiver 
* works with .py script to channel the CSI data into csi_data.csv file
* Notes:
*    - Make sure to replace MAC address with the transmitter's MAC address (marked with ***)
*    - Make sure to adjust menuconfig to do the following: component confit->ESP system settings->channel for console output->custom UART
*          - This allows you to increase the baud rate to 921600
*    - To collect data, I followed these steps/commands:
*          - idf.py build
*          - idf.py flash -p COM3 monitor -b 921600 (and run the transmitter)
*          - end terminal and open a new terminal once connection has been established
*          - python collect_csi.py
*          - view data in csi_data.csv
* Tommy Robinson, 2025
*/
// #include <stdio.h>
// #include <stdint.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "nvs_flash.h"
// #include "esp_log.h"
// #include "esp_netif.h"
// #include "lwip/inet.h"
// #include "lwip/sockets.h"
// #include "lwip/netdb.h"
// #include <math.h>
// #include "driver/uart.h"
// #include "driver/adc.h"
// #include "driver/i2c.h"
// #include "driver/gpio.h"
// #include "esp_task_wdt.h"

// #define MAX_CSI_FRAMES 200     // Number of frames to buffer
// #define MAX_CSI_LEN 384   // 40 MHz channel width
// #define GSR_PIN          35
// #define REFERENCE_RESISTOR 1000000.0f  // 1M Ohm
// #define SAMPLES          10
// #define SAMPLING_PERIOD  pdMS_TO_TICKS(100)

// // struct holds the information we need from CSI frame
// typedef struct {
//     int8_t buf[MAX_CSI_LEN];
//     int len;
//     int rssi;
//     uint64_t timestamp;
// } csi_frame_t;

// static const char *TAG = "CSI_AP";
// static const char *TAG_GSR = "CSI_AP_GSR";
// static csi_frame_t csi_buffer[MAX_CSI_FRAMES]; // buffer to hold incoming CSI data
// static volatile int buffer_head = 0;
// static volatile int buffer_tail = 0; 
// static volatile uint32_t csi_count = 0; // counter of frames per second

// static void gsr_init(void)
// {
//     // Configure ADC1 for GPIO35 (ADC1_CHANNEL_7) [web:62][web:35]
//     adc1_config_width(ADC_WIDTH_BIT_12);
//     adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);  // 0–3.3V [web:61]
//     ESP_LOGI(TAG_GSR, "ESP32 GSR Sensor started on GPIO%d", GSR_PIN);
// }

// static void gsr_read_once(float *adc_val, float *skin_res, float *skin_cond)
// {
//     int raw = adc1_get_raw(ADC1_CHANNEL_7);  // one reading only [web:61]
//     float adc_avg = (float)raw;              // keep name but no averaging

//     float vcc = 3.3f;
//     float v_gsr = (adc_avg / 4095.0f) * vcc;
//     float skin_resistance = REFERENCE_RESISTOR * ((vcc + v_gsr) / (vcc - v_gsr));
//     float skin_conductance = 1000000.0f / skin_resistance;  // µS

//     if (adc_val)   *adc_val   = adc_avg;
//     if (skin_res)  *skin_res  = skin_resistance;
//     if (skin_cond) *skin_cond = skin_conductance;
// }

// // CSI callback
// static void wifi_csi_cb(void *ctx, wifi_csi_info_t *info)
// {
//     // make sure info received
//     if (!info || !info->buf) return;
//     // filter for the mac address of the sender (Hardcoded -- ***REPLACE WITH APPROPROATE MAC ADDRESS***)
//     // if(!(info->mac[0] == 0x3C && info->mac[1] == 0x8A && info->mac[2] == 0x1F &&
//     //      info->mac[3] == 0xA7 && info->mac[4] == 0xE2 && info->mac[5] == 0x40)) {
//     //     return;
//     // }
//     if(info->len !=384){ // make sure length is correct
//         return;
//     }
    
//     // increment CSI count - this stores number of frames received per second from transmitter
//     int temp = csi_count; // prevent optimization issues
//     csi_count=temp+1;

//     int next = (buffer_head + 1) % MAX_CSI_FRAMES; // continuous
//     if (next != buffer_tail) { // Only store if buffer isn't full
//         int len = info->len > MAX_CSI_LEN ? MAX_CSI_LEN : info->len;
//         // update buffer by filling struct fields
//         memcpy(csi_buffer[buffer_head].buf, info->buf, len);
//         csi_buffer[buffer_head].len = len;
//         csi_buffer[buffer_head].rssi = info->rx_ctrl.rssi;
//         csi_buffer[buffer_head].timestamp = info->rx_ctrl.timestamp;
//         buffer_head = next;
//     }
// }

// // count the number of frames received each second and log
// void fps_task(void *pvParameter)
// {
//     // log the number of CSI frames received after a second, then reset
//     while (1) {
//         uint32_t count_last = csi_count;
//         csi_count = 0;
//         vTaskDelay(pdMS_TO_TICKS(1000)); // every 1 second
//     }
// }

// // send info through serial port using buffer to ensure all data makes it through
// void uart_flush_task(void *pvParameter)
// {
//     char line[2300]; // plenty of room for 384 values + commas + RSSI
//     while (1) {
//         while (buffer_tail != buffer_head) {
//             csi_frame_t *frame = &csi_buffer[buffer_tail];

//             // Build one string for the whole frame
            
//             int pos = snprintf(line, sizeof(line), "CSI_DATA,"); // prefix
//             for (int i = 0; i < frame->len; i++) {
//                 pos += snprintf(&line[pos], sizeof(line) - pos, "%d,", frame->buf[i]);
//                 if (pos >= sizeof(line)) break; // avoid overflow
//             }
//             // Read GSR once per CSI frame (no averaging)
//             float adc_val = 0.0f, skin_res = 0.0f, skin_cond = 0.0f;
//             gsr_read_once(&adc_val, &skin_res, &skin_cond);
//             // Add RSSI and timestamp at the end
//             pos += snprintf(&line[pos], sizeof(line) - pos, "%d,%.0f,%.0f,%.2f,%llu\n", frame->rssi,adc_val,
//                             skin_res,
//                             skin_cond, frame->timestamp);

//             // Send out over UART0 in one shot
//             uart_write_bytes(UART_NUM_0, line, strlen(line)); // send the line through serial port

//             // Advance buffer tail
//             buffer_tail = (buffer_tail + 1) % MAX_CSI_FRAMES;
//         }

//         vTaskDelay(pdMS_TO_TICKS(5)); // flush interval
//     }
// }


// extern "C" void app_main(void)
// {
//     // GSR ADC init
//     gsr_init();
//     // initialize Wi-Fi
//     ESP_ERROR_CHECK(nvs_flash_init());
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     cfg.csi_enable = 1; // Enable CSI reception
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     // set to AP mode
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

//     // set AP configuration
//     wifi_config_t ap_config = {
//         .ap = {
//             .ssid = "CSI_TEST_AP", // set ssid (used by transmitter)
//             .password = "12345678", // set password (used by transmitter)
//             .ssid_len = 0,
//             .channel = 1,  // doesn't have to be 1, but TX and RX need to be the same
//             .authmode = WIFI_AUTH_WPA2_PSK,
//             .ssid_hidden = 0,
//             .max_connection = 5,
//             .beacon_interval = 100, // added to troubleshoot disconnection/reconnection issues
//             .transition_disable = 0
//         }
//     };

//     // setting IP address for AP
//     esp_netif_ip_info_t ip_info;
//     esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

//     inet_pton(AF_INET, "192.168.4.1", &ip_info.ip);
//     inet_pton(AF_INET, "255.255.255.0", &ip_info.netmask);
//     inet_pton(AF_INET, "192.168.4.1", &ip_info.gw);

//     esp_netif_set_ip_info(ap_netif, &ip_info);
//     esp_netif_dhcps_start(ap_netif);

//     esp_wifi_set_inactive_time(WIFI_IF_AP, 500); // added to troubleshoot disconnection/reconnection issues (waits longer before disconnecting)

//     // start Wi-Fi
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
//     ESP_ERROR_CHECK(esp_wifi_start());

//     // Set to channel 1
//     wifi_country_t country = {
//         .cc = "US", .schan = 1, .nchan = 11, .policy = WIFI_COUNTRY_POLICY_MANUAL
//     };
//     ESP_ERROR_CHECK(esp_wifi_set_country(&country));

//     // Set primary channel and secondary direction for HT40
//     ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_ABOVE));

//     // Enable 11n and 40 MHz for AP
//     ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_AP,
//         WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
//     ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40));

//     // turn off power save mode
//     ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

//     // CSI settings
//     wifi_csi_config_t csi_config = {
//         .lltf_en = true,
//         .htltf_en = true, // high throughput
//         .stbc_htltf2_en = true,
//         .ltf_merge_en = false,
//         .channel_filter_en = false,
//         .manu_scale = false,
//         .shift = 0
//     };

//     // set up CSI collection
//     ESP_ERROR_CHECK(esp_wifi_set_csi_config(&csi_config));
//     ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(wifi_csi_cb, NULL)); // set callback
//     ESP_ERROR_CHECK(esp_wifi_set_csi(true));


//     // uart settings - optimize serial port for high data rate
//     const uart_config_t uart_config = {
//         .baud_rate = 921600,          // increase baud rate to stop dropping data
//         .data_bits = UART_DATA_8_BITS,
//         .parity    = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
//     };

//     uart_driver_install(UART_NUM_0, 4096, 0, 0, NULL, 0);
//     uart_param_config(UART_NUM_0, &uart_config);
//     uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);


//     // xTaskCreate(fps_task, "fps_task", 2048, NULL, 5, NULL); // count frames per second
//     xTaskCreate(uart_flush_task, "uart_flush_task", 4096, NULL, 5, NULL); // send data through serial port

//     ESP_LOGI(TAG, "AP started, waiting for STA...");
//     vTaskDelay(portMAX_DELAY); // prevent main from exiting so that wifi and csi callbacks keep running
// }

// New code adding--------------------------------------------------

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <math.h>
#include "driver/uart.h"
#include "driver/adc.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"

#define MAX_CSI_FRAMES 200     // Number of frames to buffer
#define MAX_CSI_LEN 384   // 40 MHz channel width
#define GSR_PIN          35
#define SDA_PIN          23
#define SCL_PIN          22
#define REFERENCE_RESISTOR 1000000.0f  // 1M Ohm
#define ADXL_ADDR 0x53
#define ADXL_DEVID 0x00
#define ADXL_POWER 0x2D
#define ADXL_FORMAT 0x31
#define ADXL_RATE 0x2C
#define ADXL_X0 0x32

// struct holds the information we need from CSI frame
typedef struct {
    int8_t buf[MAX_CSI_LEN];
    int len;
    int rssi;
    uint64_t timestamp;
} csi_frame_t;

static const char *TAG = "CSI_AP";
static const char *TAG_GSR = "CSI_AP_GSR";
static const char *TAG_ADXL = "CSI_AP_ADXL";
static csi_frame_t csi_buffer[MAX_CSI_FRAMES]; // buffer to hold incoming CSI data
static volatile int buffer_head = 0;
static volatile int buffer_tail = 0; 
static volatile uint32_t csi_count = 0; // counter of frames per second

// ADXL345 functions (same format as GSR)
static esp_err_t i2c_setup(void)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 23,
        .scl_io_num = 22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .clk_flags = 0
    };
    cfg.master.clk_speed = 100000;
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &cfg));
    return i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

static esp_err_t adxl_wreg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_write_to_device(I2C_NUM_0, ADXL_ADDR, buf, 2, 20 / portTICK_PERIOD_MS);
}

static esp_err_t adxl_rreg(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_write_read_device(I2C_NUM_0, ADXL_ADDR, &reg, 1, buf, len, 20 / portTICK_PERIOD_MS);
}

static void adxl_config(void)
{
    uint8_t id;
    adxl_rreg(ADXL_DEVID, &id, 1);
    ESP_LOGI(TAG_ADXL, "ADXL ID: 0x%02x", id);
    adxl_wreg(ADXL_POWER, 0x08);
    adxl_wreg(ADXL_FORMAT, 0x00);
    adxl_wreg(ADXL_RATE, 0x0A);
}

static void adxl_read_once(int16_t *x_raw, int16_t *y_raw, int16_t *z_raw, 
                          float *x_g, float *y_g, float *z_g)
{
    uint8_t data[6];
    if (adxl_rreg(ADXL_X0, data, 6) == ESP_OK) {
        *x_raw = (data[1] << 8) | data[0];
        *y_raw = (data[3] << 8) | data[2];
        *z_raw = (data[5] << 8) | data[4];
        *x_g = (*x_raw) / 256.0f;
        *y_g = (*y_raw) / 256.0f;
        *z_g = (*z_raw) / 256.0f;
    } else {
        *x_raw = *y_raw = *z_raw = 0;
        *x_g = *y_g = *z_g = 0.0f;
    }
}

static void gsr_init(void)
{
    // Configure ADC1 for GPIO35 (ADC1_CHANNEL_7) [web:62][web:35]
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);  // 0–3.3V [web:61]
    ESP_LOGI(TAG_GSR, "ESP32 GSR Sensor started on GPIO%d", GSR_PIN);
}

static void gsr_read_once(float *adc_val, float *skin_res, float *skin_cond)
{
    int raw = adc1_get_raw(ADC1_CHANNEL_7);  // one reading only [web:61]
    float adc_avg = (float)raw;              // keep name but no averaging

    float vcc = 3.3f;
    float v_gsr = (adc_avg / 4095.0f) * vcc;
    float skin_resistance = REFERENCE_RESISTOR * ((vcc + v_gsr) / (vcc - v_gsr));
    float skin_conductance = 1000000.0f / skin_resistance;  // µS

    if (adc_val)   *adc_val   = adc_avg;
    if (skin_res)  *skin_res  = skin_resistance;
    if (skin_cond) *skin_cond = skin_conductance;
}

// CSI callback
static void wifi_csi_cb(void *ctx, wifi_csi_info_t *info)
{
    // make sure info received
    if (!info || !info->buf) return;
    // filter for the mac address of the sender (Hardcoded -- ***REPLACE WITH APPROPROATE MAC ADDRESS***)
    // if(!(info->mac[0] == 0x3C && info->mac[1] == 0x8A && info->mac[2] == 0x1F &&
    //      info->mac[3] == 0xA7 && info->mac[4] == 0xE2 && info->mac[5] == 0x40)) {
    //     return;
    // }
    if(info->len !=384){ // make sure length is correct
        return;
    }
    
    // increment CSI count - this stores number of frames received per second from transmitter
    int temp = csi_count; // prevent optimization issues
    csi_count=temp+1;

    int next = (buffer_head + 1) % MAX_CSI_FRAMES; // continuous
    if (next != buffer_tail) { // Only store if buffer isn't full
        int len = info->len > MAX_CSI_LEN ? MAX_CSI_LEN : info->len;
        // update buffer by filling struct fields
        memcpy(csi_buffer[buffer_head].buf, info->buf, len);
        csi_buffer[buffer_head].len = len;
        csi_buffer[buffer_head].rssi = info->rx_ctrl.rssi;
        csi_buffer[buffer_head].timestamp = info->rx_ctrl.timestamp;
        buffer_head = next;
    }
}

// count the number of frames received each second and log
void fps_task(void *pvParameter)
{
    // log the number of CSI frames received after a second, then reset
    while (1) {
        uint32_t count_last = csi_count;
        csi_count = 0;
        vTaskDelay(pdMS_TO_TICKS(1000)); // every 1 second
    }
}

// send info through serial port using buffer to ensure all data makes it through
void uart_flush_task(void *pvParameter)
{
    char line[3000]; // plenty of room for 384 values + commas + RSSI
    while (1) {
        while (buffer_tail != buffer_head) {
            csi_frame_t *frame = &csi_buffer[buffer_tail];

            // Build one string for the whole frame
            
            int pos = snprintf(line, sizeof(line), "CSI_DATA,"); // prefix
            for (int i = 0; i < frame->len; i++) {
                pos += snprintf(&line[pos], sizeof(line) - pos, "%d,", frame->buf[i]);
                if (pos >= sizeof(line)) break; // avoid overflow
            }
            // Read GSR once per CSI frame (no averaging)
            float adc_val = 0.0f, skin_res = 0.0f, skin_cond = 0.0f;
            gsr_read_once(&adc_val, &skin_res, &skin_cond);
            // Read Accelerometer once per CSI frame (same format as GSR)
            int16_t x_raw = 0, y_raw = 0, z_raw = 0;
            float x_g = 0.0f, y_g = 0.0f, z_g = 0.0f;
            adxl_read_once(&x_raw, &y_raw, &z_raw, &x_g, &y_g, &z_g);

            // Add all sensor data + CSI metadata
            pos += snprintf(&line[pos], sizeof(line) - pos, 
                           "%d,GSR,%.0f,%.0f,%.2f,ACC_RAW,%d,%d,%d,ACC_For,%.3f,%.3f,%.3f,%llu\n",
                           frame->rssi, adc_val, skin_res, skin_cond,
                           x_raw, y_raw, z_raw, x_g, y_g, z_g, frame->timestamp);

            // Send out over UART0 in one shot
            uart_write_bytes(UART_NUM_0, line, strlen(line)); // send the line through serial port

            // Advance buffer tail
            buffer_tail = (buffer_tail + 1) % MAX_CSI_FRAMES;
        }

        vTaskDelay(pdMS_TO_TICKS(5)); // flush interval
    }
}


extern "C" void app_main(void)
{
    // GSR ADC init
    gsr_init();
    ESP_ERROR_CHECK(i2c_setup());
    adxl_config();
    // initialize Wi-Fi
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.csi_enable = 1; // Enable CSI reception
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // set to AP mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    // set AP configuration
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "CSI_TEST_AP", // set ssid (used by transmitter)
            .password = "12345678", // set password (used by transmitter)
            .ssid_len = 0,
            .channel = 1,  // doesn't have to be 1, but TX and RX need to be the same
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = 0,
            .max_connection = 5,
            .beacon_interval = 100, // added to troubleshoot disconnection/reconnection issues
            .transition_disable = 0
        }
    };

    // setting IP address for AP
    esp_netif_ip_info_t ip_info;
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

    inet_pton(AF_INET, "192.168.4.1", &ip_info.ip);
    inet_pton(AF_INET, "255.255.255.0", &ip_info.netmask);
    inet_pton(AF_INET, "192.168.4.1", &ip_info.gw);

    esp_netif_set_ip_info(ap_netif, &ip_info);
    esp_netif_dhcps_start(ap_netif);

    esp_wifi_set_inactive_time(WIFI_IF_AP, 500); // added to troubleshoot disconnection/reconnection issues (waits longer before disconnecting)

    // start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Set to channel 1
    wifi_country_t country = {
        .cc = "US", .schan = 1, .nchan = 11, .policy = WIFI_COUNTRY_POLICY_MANUAL
    };
    ESP_ERROR_CHECK(esp_wifi_set_country(&country));

    // Set primary channel and secondary direction for HT40
    ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_ABOVE));

    // Enable 11n and 40 MHz for AP
    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_AP,
        WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N));
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40));

    // turn off power save mode
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    // CSI settings
    wifi_csi_config_t csi_config = {
        .lltf_en = true,
        .htltf_en = true, // high throughput
        .stbc_htltf2_en = true,
        .ltf_merge_en = false,
        .channel_filter_en = false,
        .manu_scale = false,
        .shift = 0
    };

    // set up CSI collection
    ESP_ERROR_CHECK(esp_wifi_set_csi_config(&csi_config));
    ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(wifi_csi_cb, NULL)); // set callback
    ESP_ERROR_CHECK(esp_wifi_set_csi(true));


    // uart settings - optimize serial port for high data rate
    const uart_config_t uart_config = {
        .baud_rate = 921600,          // increase baud rate to stop dropping data
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_NUM_0, 4096, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);


    // xTaskCreate(fps_task, "fps_task", 2048, NULL, 5, NULL); // count frames per second
    xTaskCreate(uart_flush_task, "uart_flush_task", 8192, NULL, 5, NULL); // send data through serial port

    ESP_LOGI(TAG, "AP started, waiting for STA...");
    vTaskDelay(portMAX_DELAY); // prevent main from exiting so that wifi and csi callbacks keep running
}