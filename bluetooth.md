# Configuración y uso de BLE (Bluetooth Low Energy)

Usaremos el stack NimBLE para Bluetooth Low Energy. Es más moderno y requiere menos recursos que la implementación anterior Bluedroid. No soporta Bluetooth Classic, por lo que si se requiere alguna funcionalidad del bluetooth clásico (stream de audio por ejemplo) deberá usarse otro módulo.

El objetivo es poder enviar información desde el celular al microcontrolador para, por ejemplo, configurar a que red wifi conectarse.

## Inicialización

Para usar el módulo, necesitamos inicializar además el almacenamiento no volatil (NVS de Non Volatile Storage), pero es algo que ya hicimos en la clase de wifi. Por simplicidad lo dejamos dentro de la función  `app_main()`.

```c
//Initialize NVS
    esp_err_t ret = nvs_flash_init();
    ESP_ERROR_CHECK(ret);
```

En el archivo "sdkconfig.defaults", será necesario agregar las definiciones pertinentes:

```
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y
CONFIG_BT_NIMBLE_50_FEATURE_SUPPORT=n
```
La última opción es para deshabilitar varias características que son opcionales y no necesitamos en este proyecto.


También es necesario agregar `bt` a la lista de "PRIV_REQUIRES" en el archivo de cmake.


Luego, podemos inicializar el módulo NimBLE. Para ello creemos un archivo bluetooth.h para organizar nuestras funciones a modo de biblioteca single header. En ese archivo agreagamos una función para inicializar el stack de nimble.
Necesitaremos incluir los siguientes headers:

```c
/* NimBLE stack APIs */
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
```

Y agregamos la siguiente función:

```c
int init_nimble()
{
    int ret = nimble_port_init();
    ESP_ERROR_CHECK(ret);

    return ret;
}
```

Ésta última función la llamamos desde `app_main()` para inicializar el módulo.

Es necesario a continuación establecer el perfil de acceso (GAP por Generict Access Profile). El mismo se utiliza para definir roles de los dispositivos (periférico, central), la publicación (cómo aparecen en algun otro dispositivo que esta buscando dispositivos bluetooth), entre otras cosas.

```c
int gap_init(const char *device_name, int16_t appearance) {
    int rc = 0;
    ble_svc_gap_init();
    
    rc = ble_svc_gap_device_name_set(device_name); 
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set device name to %s, error code: %d",
                 device_name, rc);
        return rc;
    }
    _appearance = appearance; // variable global estática
    rc = ble_svc_gap_device_appearance_set(appearance); 
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set device appearance, error code: %d", rc);
        return rc;
    }
    return rc;
}
```

Puede ver una lista de códigos para `appearance` en este (documento)[https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/Assigned_Numbers.pdf].

Al igual que con el módulo nimble, podemos llamar la función `gap_init` desde `app_main()`.

Es necesario definir tres callbacks para usar el stack de nimble como host:

* reset_cb: Se llama cuando el stack de NimBLE se reinicia.
* sync_cb: Se llama cuando el stack del host se ha sincronizado con el controlador bluetooth.
* store_status_cb: Se llama cuando cambia la información sobre el emparejamiento y vinculación.

```c
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
```

Nuevamente, llamamos `nimble_host_config_init` desde la función principal. En el callback de reset, simplemente mostramos un mensaje indicando lo que sucedión. En el callback de sync, comenzamos la el advertisement del dispostivo, para que pueda ser descubierto por otros.

La función `adv_init` inicializa el advertisment que luego se inicia con `start_advertising`, aunque no es muy útil explicarlas en detalle. De todas formas, hay algunas lineas que son de interes:

```c
adv_fields.appearance = _appearance;
adv_fields.appearance_is_present = 1;
```
En las líneas anteriores, utilizamos la apariencia seteada en `gap_init`.

Con las siguientes instrucciones seteamos el modo de connección en "undirected-connectable", y en "general-discoverable" el modo detectable.

```
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
```




Es necesario crear una tarea para bluetooth, para lo que definimos una función:

```c
void nimble_host_task(void *param) {
    /* Task entry log */
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();

    /* Clean up at exit */
    vTaskDelete(NULL);
}
```

Y luego creamos la tarea en `app_main()` con `xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);`.

## GATT Server

Un servidor GATT tiene la función de hacer accessible los datos y servicios ofrecidos por el dispositivo BLE a los clientes. De esta forma los clientes tiene una forma estandarizada de comunicarse con los dispositivos.

Lo primero que hacemos es inicializar el servidor:

```c
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
```

La variable `gatt_svr_svcs` es una arreglo de estructuras, donde cada una define un servicio disponible en el dispositivo. Con éstas estructuras le decimos a los clientes como interactuar con el dispositivo.

