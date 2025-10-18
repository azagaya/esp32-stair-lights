#pragma once

#include <mqtt_client.h>


void print_user_property(mqtt5_user_property_handle_t user_property);
void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt5_app_start(void);