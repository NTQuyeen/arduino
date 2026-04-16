#include "esp_camera.h"
#include <WiFi.h>

// ===== FreeRTOS =====
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ================= CAMERA MODEL =================
#include "board_config.h"

// ================= WIFI =================
const char *ssid = "Kepler";
const char *password = "3141527182";

void startCameraServer();
void setupLedFlash();

// ================= TASKS =================

// Task giữ stream chạy
void streamTask(void *pvParameters) {
  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Task debug
void monitorTask(void *pvParameters) {
  while (true) {
    Serial.println("📷 ESP32-CAM đang stream ổn định...");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // ===== 🔥 CONFIG CHUẨN KHÔNG MỜ + MƯỢT =====
  config.frame_size   = FRAMESIZE_VGA;   // 640x480 → rõ mặt
  config.jpeg_quality = 8;               // nét
  config.fb_count     = 2;               // mượt hơn
  config.grab_mode    = CAMERA_GRAB_LATEST;
  config.fb_location  = CAMERA_FB_IN_PSRAM;

  if (!psramFound()) {
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.fb_count = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("❌ Camera init failed");
    ESP.restart();
  }

  sensor_t *s = esp_camera_sensor_get();

  // ===== 🔥 SENSOR TUNING (KHÔNG MỜ) =====
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);

  // Ánh sáng chuẩn
  s->set_brightness(s, 0);
  s->set_contrast(s, 1);
  s->set_saturation(s, 0);

  // Độ nét (fix mờ)
  s->set_sharpness(s, 2);
  s->set_denoise(s, 0);   // 🔥 tắt để không bị blur

  // Màu sắc tự nhiên
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);

  // Auto exposure ổn định
  s->set_exposure_ctrl(s, 1);
  s->set_gain_ctrl(s, 1);

  // Giảm nhiễu nhẹ (không làm mờ)
  s->set_gainceiling(s, (gainceiling_t)4);

#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  // ===== WIFI =====
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(300 / portTICK_PERIOD_MS);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi connected");

  // ===== START STREAM =====
  startCameraServer();

  Serial.print("📷 Stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/stream");

  // ===== TASKS =====

  // Stream task (Core 1)
  xTaskCreatePinnedToCore(
    streamTask,
    "Stream Task",
    4096,
    NULL,
    1,
    NULL,
    1
  );

  // Monitor task (Core 0)
  xTaskCreatePinnedToCore(
    monitorTask,
    "Monitor Task",
    2048,
    NULL,
    1,
    NULL,
    0
  );
}

// ================= LOOP =================
void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}