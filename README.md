# Rover-to

## Descripción general

Este repositorio reúne varios proyectos de `PlatformIO` orientados a telecomunicaciones, procesamiento de señal digital (DSP), navegación por visión e integración de sensores con ESP32-S3.

Las carpetas principales son:

- `DSP_Transmisor` - firmware para transmisores y receptores de telemetría / DSP.
- `Rover` - firmware del rover de navegación basado en VIO y ESP-NOW.
- `GPS` - proyecto independiente de GPS.
- `Pantalla` - proyecto independiente de visualización.
- `Receptor` - firmware receptor para la telemetría.

## Estructura del workspace

- `DSP_Transmisor/` contiene varios entornos de compilación para:
  - `dsp_esp32_s3` - simulación DSP de señales IMU en ESP32-S3.
  - `telecom_esp32_s3` - integración de sensores y transmisión de telemetría.
  - `telecom_xiao_s3` - transmisor maestro en Seeed XIAO ESP32-S3.
  - `receiver_xiao_s3` - receptor XIAO ESP32-S3 que recibe datos por ESP-NOW y RFM69.
- `Rover/` contiene el firmware del rover con tareas FreeRTOS, visión por flujo óptico, navegación VIO y transmisión inalámbrica.

## Cómo usar

1. Abre la carpeta raíz en VS Code.
2. Selecciona el proyecto que deseas compilar (`DSP_Transmisor` o `Rover`).
3. Usa `PlatformIO > Build` o la terminal:
   - `platformio run -e dsp_esp32_s3`
   - `platformio run -e telecom_esp32_s3`
   - `platformio run -e telecom_xiao_s3`
   - `platformio run -e receiver_xiao_s3`
   - `platformio run -e seeed_xiao_esp32s3`

## Documentación específica

- `DSP_Transmisor/README.md` - explicación detallada del transmisor, receptor y modos de compilación.
- `Rover/README.md` - explicación de la arquitectura VIO, tareas y comunicaciones del rover.

---

> Nota: `sensor_payload.h` se usa como estructura de datos compartida y debe mantenerse idéntica entre transmisor y receptor para garantizar la interoperabilidad.

