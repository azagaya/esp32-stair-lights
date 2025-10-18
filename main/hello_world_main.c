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

char ssid[MAX_SSID_LEN] = "FaIn-Privada";
char password[MAX_PASS_LEN] = "radioactividad";

void app_main(void)
{

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();
    wifi_connect(ssid, password);

    mqtt5_app_start();


}
