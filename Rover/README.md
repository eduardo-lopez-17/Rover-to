# Rover

## Descripción general

Este proyecto controla un rover basado en ESP32-S3 Xiao Sense con una arquitectura de tareas FreeRTOS. El sistema usa visión por flujo óptico, IMU y navegación inercial para estimar posición y velocidad, y transmite datos por ESP-NOW.

## Arquitectura

### `src/main.cpp`

- Inicializa las subsistemas:
  - `serial_init()`
  - `wireless_com_init()`
  - `imu_init()`
  - `camera_init()`
  - `flow_init()`
  - `navigation_init()`
- Ejecuta `task_init()` y no necesita lógica adicional en `loop()`.

### `src/task.cpp`

- Crea tareas FreeRTOS para:
  - `task_imu` — lee datos del IMU periódicamente.
  - `task_camera` — captura frames para flujo óptico.
  - `task_navigation` — actualiza la navegación VIO con EKF.
  - `task_telemetry` — imprime estado de navegación en serie.
  - `task_wireless_com_tx` — transmite payloads por ESP-NOW.

## Payload y comunicaciones

### `include/sensor_payload.h`

Define los datos transmitidos en cada paquete:

- `timestamp`
- `pos_x`, `pos_y`, `vel_x`, `vel_y`, `yaw_angle`
- `flow_valid`
- campos de sensores remotos:
  - `distance_cm`, `gps_lat`, `gps_lng`
  - `env_temp_c`, `env_humidity_pct`, `env_pressure_hpa`, `env_soil_moisture_pct`
  - `pwr_voltage_v`, `pwr_current_ma`
  - `anomaly`, `vision_obj_id`, `vision_confidence`

> En el rover, los campos de sensores físicos que no existen en este nodo se inicializan a cero.

### `src/wireless_com.cpp`

- Inicializa ESP-NOW con una clave AES-128 `PlantioSecKey123`.
- Define la dirección MAC del receptor en `config.h`.
- Transmite `SensorPayload` cada `WIRELESS_COM_PERIOD_MS`.

## Configuración

### `include/config.h`

- Activa y ajusta tareas claramente:
  - `ENABLE_IMU`
  - `ENABLE_CAMERA`
  - `ENABLE_NAVIGATION`
  - `ENABLE_TELEMETRY`
  - `ENABLE_WIRELESS_COM`
- Define prioridades y periodos:
  - `IMU_PERIOD_MS = 2`
  - `CAMERA_PERIOD_MS = 20`
  - `NAVIGATION_PERIOD_MS = 33`
  - `TELEMETRY_PERIOD_MS = 100`
  - `WIRELESS_COM_PERIOD_MS = 100`
- Ajustes VIO y EKF para robustez de navegación visual.

### `include/pins.h`

Contiene el mapeo de pines físicos para la placa objetivo.

## Flujo de funcionamiento

1. El IMU se lee continuamente y alimenta el filtro de navegación.
2. La cámara calcula el flujo óptico.
3. `navigation_update()` combina IMU y flujo óptico para estimar posición y orientación.
4. El rover transmite el estado estimado por ESP-NOW.
5. El módulo de telemetría imprime el estado en serie para depuración.

## Compilación

Desde la carpeta `Rover`:

- `platformio run -e seeed_xiao_esp32s3`

## Ajustes de despliegue

- Actualiza `WIRELESS_COM_BROADCAST_ADDRESS` en `config.h` si tu receptor tiene otra MAC.
- Cambia la clave AES-128 en `wireless_com.cpp` y la configuración del receptor para comunicación segura.
- Si no usas cámara o IMU, desactiva las macros correspondientes para reducir consumo.
