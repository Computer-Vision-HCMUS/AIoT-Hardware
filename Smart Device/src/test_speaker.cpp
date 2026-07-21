#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>
#include "pins_config.h"

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Speaker-only test starting...");

  // Bật ampli
  pinMode(I2S_SPK_SD_PIN, OUTPUT);
  digitalWrite(I2S_SPK_SD_PIN, HIGH);

  // Cấu hình I2S giống hệt AudioManager::init() phần loa
  i2s_config_t spk_cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = AUDIO_DMA_BUF_COUNT,
    .dma_buf_len = AUDIO_DMA_BUF_LEN,
    .use_apll = false,
    .tx_desc_auto_clear = true,
  };
  i2s_driver_install(I2S_SPK_PORT, &spk_cfg, 0, NULL);

  i2s_pin_config_t spk_pins = {
    .bck_io_num = I2S_SPK_BCLK_PIN,
    .ws_io_num = I2S_SPK_LRCLK_PIN,
    .data_out_num = I2S_SPK_DOUT_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE,
  };
  i2s_set_pin(I2S_SPK_PORT, &spk_pins);

  Serial.println("Playing 440Hz tone for 3 seconds...");

  int16_t buf[512];
  for (int i = 0; i < 512; i++) {
    buf[i] = (int16_t)(8000 * sin(2 * PI * 440 * i / AUDIO_SAMPLE_RATE));
  }

  size_t written;
  for (int loop = 0; loop < 94; loop++) {  // ~3s ở 16kHz, buf 512
    i2s_write(I2S_SPK_PORT, buf, sizeof(buf), &written, portMAX_DELAY);
  }

  Serial.println("Done. Did you hear a tone?");
}

void loop() {}