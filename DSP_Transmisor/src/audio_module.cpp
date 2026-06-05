#include "audio_module.h"
#include "board_config.h"

#if USE_AUDIO
#include <driver/i2s.h>
#include "SD.h"

/* XIAO ESP32-S3 Sense built-in PDM microphone pins */
#define MIC_CLK_PIN      42
#define MIC_DATA_PIN     41
#define MIC_I2S_PORT     I2S_NUM_0
#define MIC_SAMPLE_RATE  16000
#define MIC_RECORD_SEC   5
#define MIC_DMA_BUF_LEN  1024
#define MIC_DMA_BUF_CNT  8

/* Total samples and byte count for one recording */
#define MIC_TOTAL_SAMPLES (MIC_SAMPLE_RATE * MIC_RECORD_SEC)
#define MIC_TOTAL_BYTES   (MIC_TOTAL_SAMPLES * sizeof(int16_t))

static bool     s_mic_ok;
static int      s_rec_count;

/* -------------------------------------------------------------------------
 * WAV header (44 bytes, PCM mono 16-bit)
 * --------------------------------------------------------------------- */
static void write_wav_header(File &f, uint32_t pcm_bytes)
{
    uint32_t chunk_size   = 36 + pcm_bytes;
    uint32_t byte_rate    = MIC_SAMPLE_RATE * 1 * 2; /* SR * ch * bytes/sample */
    uint16_t block_align  = 2;
    uint16_t bits         = 16;
    uint16_t audio_fmt    = 1; /* PCM */
    uint16_t num_channels = 1;

    f.write((const uint8_t *)"RIFF",         4);
    f.write((const uint8_t *)&chunk_size,    4);
    f.write((const uint8_t *)"WAVE",         4);
    f.write((const uint8_t *)"fmt ",         4);
    uint32_t sub1 = 16;
    f.write((const uint8_t *)&sub1,          4);
    f.write((const uint8_t *)&audio_fmt,     2);
    f.write((const uint8_t *)&num_channels,  2);
    uint32_t sr = MIC_SAMPLE_RATE;
    f.write((const uint8_t *)&sr,            4);
    f.write((const uint8_t *)&byte_rate,     4);
    f.write((const uint8_t *)&block_align,   2);
    f.write((const uint8_t *)&bits,          2);
    f.write((const uint8_t *)"data",         4);
    f.write((const uint8_t *)&pcm_bytes,     4);
}
#endif /* USE_AUDIO */

/* =========================================================================
 * audio_init
 * ========================================================================= */
void audio_init(void)
{
#if USE_AUDIO
    i2s_config_t cfg = {};
    cfg.mode              = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    cfg.sample_rate       = MIC_SAMPLE_RATE;
    cfg.bits_per_sample   = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format    = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_PCM_SHORT;
    cfg.intr_alloc_flags  = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count     = MIC_DMA_BUF_CNT;
    cfg.dma_buf_len       = MIC_DMA_BUF_LEN;
    cfg.use_apll          = false;

    i2s_pin_config_t pins = {};
    pins.bck_io_num       = I2S_PIN_NO_CHANGE;
    pins.ws_io_num        = MIC_CLK_PIN;
    pins.data_out_num     = I2S_PIN_NO_CHANGE;
    pins.data_in_num      = MIC_DATA_PIN;

    esp_err_t err = i2s_driver_install(MIC_I2S_PORT, &cfg, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] ERROR: I2S driver install failed (%d)\n", err);
        return;
    }
    err = i2s_set_pin(MIC_I2S_PORT, &pins);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] ERROR: I2S pin config failed (%d)\n", err);
        i2s_driver_uninstall(MIC_I2S_PORT);
        return;
    }
    s_mic_ok = true;
    Serial.printf("[AUDIO] OK — PDM mic ready, %d Hz, %ds per recording\n",
                  MIC_SAMPLE_RATE, MIC_RECORD_SEC);
#else
    Serial.println("[AUDIO] disabled (DNP)");
#endif
}

/* =========================================================================
 * audio_record_to_sd
 * Records MIC_RECORD_SEC seconds of PDM audio, saves to SD as WAV.
 * Returns file path on success, empty string on any failure.
 * All failure paths print an error and return — nothing blocks the caller.
 * ========================================================================= */
String audio_record_to_sd(void)
{
#if USE_AUDIO
    if (!s_mic_ok) {
        Serial.println("[AUDIO] ERROR: mic not initialized");
        return "";
    }

    /* Allocate recording buffer in PSRAM if available */
    int16_t *buf = (int16_t *)ps_malloc(MIC_TOTAL_BYTES);
    if (!buf) {
        buf = (int16_t *)malloc(MIC_TOTAL_BYTES);
        if (!buf) {
            Serial.println("[AUDIO] ERROR: not enough RAM for recording buffer");
            return "";
        }
    }

    /* Record — drain DMA into buffer in chunks */
    size_t   bytes_read = 0;
    size_t   total_read = 0;
    uint32_t t0         = millis();

    i2s_start(MIC_I2S_PORT);
    while (total_read < MIC_TOTAL_BYTES) {
        i2s_read(MIC_I2S_PORT,
                 (char *)buf + total_read,
                 MIC_TOTAL_BYTES - total_read,
                 &bytes_read,
                 portMAX_DELAY);
        total_read += bytes_read;
    }
    i2s_stop(MIC_I2S_PORT);

    Serial.printf("[AUDIO] recorded %u bytes in %lums\n",
                  (unsigned)total_read, (unsigned long)(millis() - t0));

    /* Save to SD — skip gracefully if SD not mounted */
    String path = "/audio_" + String(++s_rec_count) + ".wav";

    if (!SD.begin()) {
        Serial.println("[AUDIO] SD not available — recording discarded");
        free(buf);
        return "";
    }

    File f = SD.open(path.c_str(), FILE_WRITE);
    if (!f) {
        Serial.printf("[AUDIO] ERROR: cannot open %s for writing\n", path.c_str());
        free(buf);
        return "";
    }

    write_wav_header(f, (uint32_t)MIC_TOTAL_BYTES);
    f.write((const uint8_t *)buf, MIC_TOTAL_BYTES);
    f.close();
    free(buf);

    Serial.printf("[AUDIO] saved: %s (%.1f KB)\n",
                  path.c_str(), MIC_TOTAL_BYTES / 1024.0f);
    return path;
#else
    return "";
#endif
}
