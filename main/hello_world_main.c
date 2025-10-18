/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "wifi.h"
#include "mqtt.h"

#include <nvs_flash.h>

#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 64
#define MAX_SSIDS 2

char ssid[MAX_SSIDS][MAX_SSID_LEN] = {"DAVITEL_32117_2.4", "FaIn-Privada"};
char password[MAX_SSIDS][MAX_PASS_LEN] = {"20355960936","radioactividad"};

void app_main(void)
{

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();
    int error;
    int net_idx = -1;
    do
    {
        net_idx++;
        error = wifi_connect(ssid[net_idx], password[net_idx]);
    } while (error && net_idx < MAX_SSIDS);

    if (error)
    {
        ESP_LOGE(TAG, "COULD NOT CONECT TO INTERNET");
    }
    else
    {
        mqtt5_app_start();
    }

}
