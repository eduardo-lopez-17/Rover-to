# DSP_Transmisor

## Resumen

Este proyecto contiene firmware para estaciones de transmisión y recepción de telemetría enfocado en DSP (procesamiento digital de señales), sensores ambientales y enlaces inalámbricos para ESP32-S3.

### Componentes principales

- `main_dsp.cpp`
  - Genera datos sintéticos de posición/velocidad/ángulo a 100 Hz.
  - Simula anomalías de choque cuando la señal supera un umbral.
  - Transmite los datos por ESP-NOW usando una estructura `SensorPayload` empaquetada.

- `main_telecom.cpp`
  - Lee sensores reales como BNO085, BME280, INA219, BH1750, GPS, ultrasonido y humedad de suelo.
  - Maneja un OLED SH1106 para mostrar datos en tiempo real.
  - Incluye soporte opcional para `SIM800` y cámara/SD si la configuración lo habilita.
  - Envía datos por ESP-NOW y RFM69.

- `main_receiver.cpp`
  - Recibe datos de telemetría por ESP-NOW y RFM69.
  - Imprime paquetes recibidos en el puerto serie.
  - Supervisa pérdida de enlace y estados de transmisión.

## Entornos de compilación

El archivo `platformio.ini` define varios `env`:

- `[env:dsp_esp32_s3]`
  - Para el reto DSP con ESP32-S3.
  - Compila `main_dsp.cpp`.

- `[env:telecom_esp32_s3]`
  - Para integración de sensores y telecomunicaciones en ESP32-S3.
  - Compila `main_telecom.cpp`.

- `[env:telecom_xiao_s3]`
  - Para el transmisor maestro en Seeed XIAO ESP32-S3.
  - Activa PSRAM y define partición `huge_app.csv`.

- `[env:receiver_xiao_s3]`
  - Para el receptor en Seeed XIAO ESP32-S3.
  - Compila `main_receiver.cpp` y `rfm69_module.cpp`.

## Configuración y dependencias

- Usa `board_config.h` para habilitar o deshabilitar módulos:
  - `USE_ESPNOW`, `USE_RFM69`, `USE_GPS`, `USE_BME280`, `USE_INA219`, `USE_BNO085`, `USE_ULTRASONIC`, `USE_BH1750`, `USE_SOIL`, `USE_SIM800`, `USE_VISION_SD`.
- `sensor_payload.h` define la trama de datos común.
- `pin_map.h` y `board_config.h` contienen ajustes de pines y direcciones I2C.

## Flujo de datos

1. El transmisor lee o simula datos de sensores.
2. Llena una estructura `SensorPayload`:
   - `timestamp`
   - `pos_x`, `pos_y`, `yaw_angle`
   - `distance_cm`, `gps_lat`, `gps_lng`
   - `env_temp_c`, `env_humidity_pct`, `env_pressure_hpa`, `env_soil_moisture_pct`
   - `pwr_voltage_v`, `pwr_current_ma`
   - `anomaly`, `vision_obj_id`, `vision_confidence`
3. Envía el paquete por ESP-NOW (y opcionalmente RFM69).
4. El receptor imprime la telemetría y supervisa el estado de enlace.

## Cómo compilar

Desde la raíz del proyecto `DSP_Transmisor`:

- `platformio run -e dsp_esp32_s3`
- `platformio run -e telecom_esp32_s3`
- `platformio run -e telecom_xiao_s3`
- `platformio run -e receiver_xiao_s3`

## Notas útiles

- El código puede funcionar en modo de simulación sin hardware físico si el sensor BNO085 está deshabilitado.
- La clave AES-128 en `board_config.h` debe coincidir en TX y RX para la comunicación cifrada.
- El receptor espera que el tamaño del paquete recibido coincida con `sizeof(SensorPayload)`.
