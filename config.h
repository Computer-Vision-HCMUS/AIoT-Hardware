#pragma once

// ============================================================================
// Pin assignments - Tương thích hoàn toàn cho cả ESP32-S và ESP32-S3
// ============================================================================

#pragma once

// Pin assignments cho bản mạch kết hợp ESP32-S / S3
static const int PIN_BUTTON_NEXT   = 12;
static const int PIN_BUTTON_SELECT = 13;
static const int PIN_BUTTON_UP     = 14;
static const int PIN_BUTTON_DOWN   = 15;

// Màn hình TFT LCD ILI9341 SPI
static const int TFT_CS   = 5;
static const int TFT_DC   = 2;
static const int TFT_RST  = 4;
static const int TFT_MOSI = 23;
static const int TFT_SCLK = 18;

// SỬA ĐỔI: Chuyển cảm biến ánh sáng sang đọc Analog (ADC)
static const int PIN_LIGHT_SENSOR_ANALOG = 36; // Nối vào chân AO của cảm biến ánh sáng

// Cảm biến Nhiệt độ DHT11
static const int PIN_DHT11_DATA = 33;

// Microphone INMP441 (I2S)
static const int PIN_MIC_I2S_SCK = 26;
static const int PIN_MIC_I2S_WS  = 27;
static const int PIN_MIC_I2S_SD  = 39; 

// Module RTC DS3231 (Vẫn giữ I2C mặc định phần cứng nếu sau này lắp thêm)
static const int PIN_RTC_SDA = 21;
static const int PIN_RTC_SCL = 22;

// ============================================================================
// Cấu hình các thông số mặc định 
// ============================================================================

// Pomodoro defaults [cite: 80]
static const int DEFAULT_FOCUS_MINUTES = 25;
static const int DEFAULT_BREAK_MINUTES = 5;
static const int MIN_FOCUS_MINUTES     = 5;
static const int MAX_FOCUS_MINUTES     = 120;
static const int MIN_BREAK_MINUTES     = 1;
static const int MAX_BREAK_MINUTES     = 30;

// Sleep monitoring [cite: 99]
static const int SLEEP_SAMPLE_INTERVAL_MS = 60000; // 60 giây lấy mẫu 1 lần
static const int MAX_HISTORY_RECORDS      = 1000;
static const int HISTORY_RETENTION_DAYS   = 30;