#include <Arduino.h>
#include <driver/i2s.h>
#include "pins_config.h"

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Mic-only test starting...");

  i2s_config_t mic_cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = AUDIO_DMA_BUF_COUNT,
    .dma_buf_len = AUDIO_DMA_BUF_LEN,
    .use_apll = false,
    .tx_desc_auto_clear = false,
  };

  esp_err_t err1 = i2s_driver_install(I2S_MIC_PORT, &mic_cfg, 0, NULL);
  Serial.printf("i2s_driver_install: %s\n", err1 == ESP_OK ? "OK" : "FAILED");

  i2s_pin_config_t mic_pins = {
    .bck_io_num = I2S_MIC_SCK_PIN,
    .ws_io_num = I2S_MIC_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SD_PIN,
  };

  esp_err_t err2 = i2s_set_pin(I2S_MIC_PORT, &mic_pins);
  Serial.printf("i2s_set_pin: %s\n", err2 == ESP_OK ? "OK" : "FAILED");

  // ====== THÊM ĐOẠN NÀY VÀO ĐÂY ======
  delay(200);  // để I2S peripheral chạy ổn định

  unsigned long sckPulse = pulseIn(I2S_MIC_SCK_PIN, HIGH, 100000UL);
  Serial.printf("SCK pulse width (us): %lu %s\n", sckPulse,
                sckPulse > 0 ? "-> co xung, SCK OK" : "-> KHONG co xung!");

    Serial.println("Polling WS pin for 500ms...");
    int wsChanges = 0;
    int lastState = digitalRead(I2S_MIC_WS_PIN);
    unsigned long startMs = millis();
    while (millis() - startMs < 500) {
    int state = digitalRead(I2S_MIC_WS_PIN);
    if (state != lastState) {
        wsChanges++;
        lastState = state;
    }
    }
    Serial.printf("WS state changes in 500ms: %d %s\n", wsChanges,
                wsChanges > 100 ? "-> WS dang chay" : "-> WS KHONG chay hoac qua cham");
    // ====== KẾT THÚC ĐOẠN THÊM ======

  Serial.println("Reading mic... speak or tap near it now.");
}

void loop() {
  int32_t readBuf[512];
  size_t bytesRead = 0;

  esp_err_t err = i2s_read(I2S_MIC_PORT, readBuf, sizeof(readBuf), &bytesRead, pdMS_TO_TICKS(200));

  if (err != ESP_OK || bytesRead == 0) {
    Serial.println("(no data read)");
    delay(200);
    return;
  }

  int32_t peak = 0;
  int samples = bytesRead / sizeof(int32_t);
  for (int i = 0; i < samples; i++) {
    int16_t s16 = (int16_t)(readBuf[i] >> 16);
    int32_t a = s16 < 0 ? -s16 : s16;
    if (a > peak) peak = a;
  }

  Serial.printf("peak = %ld  (raw[0] = %ld)\n", (long)peak, (long)readBuf[0]);
  delay(150);
}