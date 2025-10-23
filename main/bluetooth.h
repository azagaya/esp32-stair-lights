#pragma once

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "services/gap/ble_svc_gap.h"

#include "host/ble_gatt.h"
#include "services/gatt/ble_svc_gatt.h"


static uint8_t own_addr_type;
static uint8_t addr_val[6] = {0};

static uint16_t _appearance;

void ble_store_config_init(void);

inline static void format_addr(char *addr_str, uint8_t addr[]) {
    sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1],
            addr[2], addr[3], addr[4], addr[5]);
}

esp_err_t init_nimble()
{
    esp_err_t ret = nimble_port_init();
    ESP_ERROR_CHECK(ret);

    return ret;
}

int gap_init(const char *device_name, int16_t appearance) {
    int rc = 0;
    ble_svc_gap_init();
    
    rc = ble_svc_gap_device_name_set(device_name); 
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set device name to %s, error code: %d",
                 device_name, rc);
        return rc;
    }

    rc = ble_svc_gap_device_appearance_set(appearance); 
    _appearance = appearance; // almaceno en variable global para luego utilizarla en el advertisment
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set device appearance, error code: %d", rc);
        return rc;
    }
    return rc;
}

static void print_conn_desc(struct ble_gap_conn_desc *desc) {
    /* Local variables */
    char addr_str[18] = {0};

    /* Connection handle */
    ESP_LOGI(TAG, "connection handle: %d", desc->conn_handle);

    /* Local ID address */
    format_addr(addr_str, desc->our_id_addr.val);
    ESP_LOGI(TAG, "device id address: type=%d, value=%s",
             desc->our_id_addr.type, addr_str);

    /* Peer ID address */
    format_addr(addr_str, desc->peer_id_addr.val);
    ESP_LOGI(TAG, "peer id address: type=%d, value=%s", desc->peer_id_addr.type,
             addr_str);

    /* Connection info */
    ESP_LOGI(TAG,
             "conn_itvl=%d, conn_latency=%d, supervision_timeout=%d, "
             "encrypted=%d, authenticated=%d, bonded=%d\n",
             desc->conn_itvl, desc->conn_latency, desc->supervision_timeout,
             desc->sec_state.encrypted, desc->sec_state.authenticated,
             desc->sec_state.bonded);
}

static int gap_event_handler(struct ble_gap_event *event, void *arg);

static void start_advertising(void) {
    /* Local variables */
    int rc = 0;
    const char *name;
    struct ble_hs_adv_fields adv_fields = {0};
    struct ble_hs_adv_fields rsp_fields = {0};
    struct ble_gap_adv_params adv_params = {0};

    /* Set advertising flags */
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Set device name */
    name = ble_svc_gap_device_name();
    adv_fields.name = (uint8_t *)name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;

    /* Set device tx power */
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;

    /* Set device appearance */
    adv_fields.appearance = _appearance;
    adv_fields.appearance_is_present = 1;

    /* Set device LE role */
    adv_fields.le_role = 0; //periférico
    adv_fields.le_role_is_present = 1;

    /* Set advertiement fields */
    rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
        return;
    }

    /* Set device address */
    rsp_fields.device_addr = addr_val;
    rsp_fields.device_addr_type = own_addr_type;
    rsp_fields.device_addr_is_present = 1;

    rsp_fields.adv_itvl = BLE_GAP_ADV_ITVL_MS(500);
    rsp_fields.adv_itvl_is_present = 1;

    /* Set scan response fields */
    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set scan response data, error code: %d", rc);
        return;
    }

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(500);
    adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(510);    

    /* Start advertising */
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                           gap_event_handler, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start advertising, error code: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "advertising started!");
}

static int gap_event_handler(struct ble_gap_event *event, void *arg) {
    /* Local variables */
    int rc = 0;
    struct ble_gap_conn_desc desc;

    /* Handle different GAP event */
    switch (event->type) {

    /* Connect event */
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        ESP_LOGI(TAG, "connection %s; status=%d",
                 event->connect.status == 0 ? "established" : "failed",
                 event->connect.status);

        /* Connection succeeded */
        if (event->connect.status == 0) {
            /* Check connection handle */
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            if (rc != 0) {
                ESP_LOGE(TAG,
                         "failed to find connection by handle, error code: %d",
                         rc);
                return rc;
            }

            /* Print connection descriptor */
            print_conn_desc(&desc);

            /* Try to update connection parameters */
            struct ble_gap_upd_params params = {.itvl_min = desc.conn_itvl,
                                                .itvl_max = desc.conn_itvl,
                                                .latency = 3,
                                                .supervision_timeout =
                                                    desc.supervision_timeout};
            rc = ble_gap_update_params(event->connect.conn_handle, &params);
            if (rc != 0) {
                ESP_LOGE(
                    TAG,
                    "failed to update connection parameters, error code: %d",
                    rc);
                return rc;
            }
        }
        /* Connection failed, restart advertising */
        else {
            start_advertising();
        }
        return rc;

    /* Disconnect event */
    case BLE_GAP_EVENT_DISCONNECT:
        /* A connection was terminated, print connection descriptor */
        ESP_LOGI(TAG, "disconnected from peer; reason=%d",
                 event->disconnect.reason);

        /* Restart advertising */
        start_advertising();
        return rc;

    /* Connection parameters update event */
    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        ESP_LOGI(TAG, "connection updated; status=%d",
                 event->conn_update.status);

        /* Print connection descriptor */
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        if (rc != 0) {
            ESP_LOGE(TAG, "failed to find connection by handle, error code: %d",
                     rc);
            return rc;
        }
        print_conn_desc(&desc);
        return rc;
    }

    return rc;
}

void adv_init(void) {
    int rc = 0;
    char addr_str[18] = {0};

    /* Make sure we have proper BT identity address set */
    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "device does not have any available bt address!");
        return;
    }

    /* Figure out BT address to use while advertising */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to infer address type, error code: %d", rc);
        return;
    }

    /* Copy device address to addr_val */
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to copy device address, error code: %d", rc);
        return;
    }
    format_addr(addr_str, addr_val);
    ESP_LOGI(TAG, "device address: %s", addr_str);

    /* Start advertising. */
    start_advertising();
}

static void on_stack_reset(int reason) {
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

static void on_stack_sync(void) {
    adv_init();
}

void nimble_host_config_init() {
    /* Set host callbacks */
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configuration */
    ble_store_config_init();

}

static const ble_uuid16_t light_sensor_svc_uuid = BLE_UUID16_INIT(0xFF01); // uuid custom
static const ble_uuid16_t ambient_light_chr_uuid = BLE_UUID16_INIT(0x2A77); // luminous intensity uuid

static uint16_t ambient_light_cr_val_handle;
static int8_t ambient_light_value;

static const ble_uuid16_t command_svc_uuid = BLE_UUID16_INIT(0xFF02); 
static const ble_uuid16_t command_tx_chr_uuid = BLE_UUID16_INIT(0xFF03);

static uint16_t command_tx_chr_val_handle;

static int ambient_light_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
        /* Local variables */
    int rc;

    /* Handle access events */
    /* Note: Heart rate characteristic is read only */
    switch (ctxt->op) {

    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_READ_CHR:
        if (attr_handle == ambient_light_cr_val_handle) {
            /* Update access buffer value */
            ambient_light_value = 100 + rand() % 10;
            rc = os_mbuf_append(ctxt->om, &ambient_light_value,
                                sizeof(ambient_light_value));
        }
        else
        {
            rc = BLE_ATT_ERR_UNLIKELY;
        }
        break;

    /* Unknown event */
    default:
        rc = BLE_ATT_ERR_UNLIKELY;
    }

    return rc;
}

#define MAX_COMMAND_LENGTH 256
static char received_command[MAX_COMMAND_LENGTH + 1] = {0}; 
static uint16_t command_len = 0;

static int command_tx_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (attr_handle != command_tx_chr_val_handle) return BLE_ATT_ERR_UNLIKELY; 
    
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            if (ctxt->om->om_len > MAX_COMMAND_LENGTH) {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            
            // 2. Copiar los datos al buffer local (la cadena de comando)
            command_len = ctxt->om->om_len;
            if (command_len > 0) {
                // Copiar hasta el límite y asegurar terminación nula (NULL)
                memcpy(received_command, ctxt->om->om_data, command_len);
                received_command[command_len] = '\0'; 
            } else {
                received_command[0] = '\0';
            }
            BLE_HS_LOG(INFO, "Comando recibido (conn %d): %s\n", conn_handle, received_command);
            
            return 0; // Éxito en la operación

        default:
            return BLE_ATT_ERR_UNLIKELY;
    }
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /* Heart rate service */
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &light_sensor_svc_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {/* Heart rate characteristic */
              .uuid = &ambient_light_chr_uuid.u,
              .access_cb = ambient_light_cb,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
              .val_handle = &ambient_light_cr_val_handle},
             {
                 0, /* No more characteristics in this service. */
             }}},

    /* Automation IO service */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &command_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){/* LED characteristic */
                                        {
                                            .uuid = &command_tx_chr_uuid.u,
                                            .access_cb = command_tx_chr_access,
                                            .flags = BLE_GATT_CHR_F_WRITE,
                                            .val_handle = &command_tx_chr_val_handle
                                        },
                                        {
                                            0
                                        }
                                    },
    },

    {
        0, /* No more services. */
    },
};

int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}


void nimble_host_task(void *param) {
    /* Task entry log */
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();

    /* Clean up at exit */
    vTaskDelete(NULL);
}
