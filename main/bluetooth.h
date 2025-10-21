#pragma once

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

int init_nimble()
{
    int ret = nimble_port_init();
    ESP_ERROR_CHECK(ret);

    return ret;
}