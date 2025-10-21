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

También es necesario agregar `bt` a la lista de "PRIV_REQUIRES" en el archivo de cmake.

```c
    ret = nimble_port_init();
    ESP_ERROR_CHECK(ret);
```

