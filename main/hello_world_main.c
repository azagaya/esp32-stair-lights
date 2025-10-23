/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "wifi.h"
#include "mqtt.h"
#include "bluetooth.h"

#include <nvs_flash.h>

#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 64
#define MAX_SSIDS 2

char ssid[MAX_SSIDS][MAX_SSID_LEN] = {"FaIn-Privada","DAVITEL_32117_2.4"};
char password[MAX_SSIDS][MAX_PASS_LEN] = {"radioactividad","20355960936"};

#define DEVICE_NAME 
#define BLE_APPEARANCE 



void app_main(void)
{

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);

    
    int error;

    init_nimble();
    gap_init("StairLights", 0x0595); // código para generic light controller;
    gatt_svc_init();
    nimble_host_config_init();


    xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);

    wifi_init_sta();
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
